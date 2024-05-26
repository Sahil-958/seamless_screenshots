#include <ImageMagick-7/Magick++.h>
#include <ImageMagick-7/Magick++/Image.h>
#include <ImageMagick-7/Magick++/Include.h>
#include <cstddef>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std;

struct Options {
  int padding = 80;
  int radius = 20;
  int angle = 9;
  string format = "png";
  string outputFileName = "output";
  string gradientFromColor = "";
  string gradientToColor = "";
  int gradientAngle = 90;
  vector<string> images;
  int imageHeight = 0;
  int imageWidth = 0;
  Magick::Image rotatedAndMergedImage;
  Magick::Image mask;
  Magick::Image gradientImage;
  Magick::Image roundedImage;
  Magick::Image finalImage;
};

void clear_line() {
  // ANSI escape code to clear the current line
  std::cout << "\33[2K\r" << std::flush;
}

void update_status(const std::string &status) {
  clear_line();
  std::cout << status << std::flush;
}

void usage() {
  cout << "Usage: program_name [options] <image1> <image2>" << endl;
  cout << "Options:" << endl;
  cout << "  -h Prints this usage" << endl;
  cout << "  -p <padding>     Padding size for the gradient background "
          "(default: 80)"
       << endl;
  cout << "  -r <radius>      Corner radius for the output image (default: 20)"
       << endl;
  cout << "  -a <angle>       Angle to rotate the output image by (default: 9)"
       << endl;
  cout << "  -t <png|jpeg|...|tiff>  File Format CAUTION: JPEG might not "
          "work very well"
       << endl;
  cout << "  -o <file Name>   Name of the output file (default: output.png)"
       << endl;
  cout << "  -gf <color> Speicfy Gradient fromColor color (default: "
          "generated from first supplied image)"
       << endl;
  cout << "  -gt <color> Speicfy Gradient toColor color (default: "
          "generated from last supplied image)"
       << endl;
  cout << "  -ga <angle> Speicfy Gradient Angle (default: 180-angle)" << endl;
  cout << "EXAMPLE:" << endl;
  cout << "program_name -r 25 -p 32 -a 9 -gf \"#ff0000\" -gt \"#00ff00\" "
          "-ga 90 -t png -o myoutput images*.png"
       << endl;
  exit(1);
}

void parseArguments(int argc, char *argv[], Options &options) {
  for (int i = 1; i < argc; ++i) {
    string arg = argv[i];

    if (arg == "-h") {
      usage();
    } else if (arg == "-p") {
      if (i + 1 < argc) {
        options.padding = stoi(argv[++i]);
      }
    } else if (arg == "-r") {
      if (i + 1 < argc) {
        options.radius = stoi(argv[++i]);
      }
    } else if (arg == "-a") {
      if (i + 1 < argc) {
        options.angle = stoi(argv[++i]);
      }
    } else if (arg == "-t") {
      if (i + 1 < argc) {
        options.format = argv[++i];
      }
    } else if (arg == "-o") {
      if (i + 1 < argc) {
        options.outputFileName = argv[++i];
      }
    } else if (arg == "-gf") {
      if (i + 1 < argc) {
        options.gradientFromColor = argv[++i];
      }
    } else if (arg == "-gt") {
      if (i + 1 < argc) {
        options.gradientToColor = argv[++i];
      }
    } else if (arg == "-ga") {
      if (i + 1 < argc) {
        options.gradientAngle = stoi(argv[++i]);
      }
    } else {
      options.images.push_back(arg);
    }
  }
}

void printParsedArguments(Options &options) {
  cout << "Parsed Arguments:" << endl;
  cout << "Padding: " << options.padding << endl;
  cout << "Radius: " << options.radius << endl;
  cout << "Angle: " << options.angle << endl;
  cout << "Format: " << options.format << endl;
  cout << "Output File Name: " << options.outputFileName << endl;
  if (options.gradientFromColor.empty()) {
    cout << "Gradient From Color: Generated from first image" << endl;
  } else {
    cout << "Gradient From Color: " << options.gradientFromColor << endl;
  }
  if (options.gradientToColor.empty()) {
    cout << "Gradient To Color: Generated from last image" << endl;
  } else {
    cout << "Gradient To Color: " << options.gradientToColor << endl;
  }
  cout << "Gradient Angle: " << options.gradientAngle << endl;
  cout << "Images:" << options.images.size() << endl;
}

void rotateImages(string imagePath, Options &options,
                  vector<Magick::Image> &images, int index) {
  Magick::Image image;
  try {
    image.read(imagePath); // Read the image
    image.backgroundColor("none");
    image.rotate(options.angle);
    image.trim();

    size_t imageWidth = image.columns();
    size_t imageHeight = image.rows();
    size_t posX = image.page().xOff();
    size_t posY = image.page().yOff();

    size_t cropHeight = imageHeight / options.images.size();
    size_t position = index * cropHeight;
    size_t newPositionY = position + posY;

    image.crop(Magick::Geometry(imageWidth, cropHeight, posX, newPositionY));
    images[index] = image;
  } catch (Magick::Error &error) {
    cerr << "Error reading image: " << error.what() << endl;
  }
}

void rotateAndMergeImages(Options &options, vector<string> &imageArgs) {
  vector<Magick::Image> images(imageArgs.size());
  vector<thread> threads;
  for (int i = 0; i < images.size(); i++) {
    threads.emplace_back(rotateImages, imageArgs[i], ref(options), ref(images),
                         i);
  }

  // wait for all threads to finish
  for (auto &thread : threads) {
    thread.join();
  }

  update_status("Rotating and Merging Images: Done Rotating");
  update_status("Rotating and Merging Images: Mergeing Now");
  // Merge images
  Magick::Image result;
  Magick::appendImages(&result, images.begin(), images.end(), true);
  result.rotate(-options.angle);
  result.trim();
  options.rotatedAndMergedImage = result;
  options.finalImage = result;
  update_status("Rotating and Merging Images: Completed");
}

void createMask(Options &options) {
  size_t width = options.rotatedAndMergedImage.columns();
  size_t height = options.rotatedAndMergedImage.rows();
  Magick::Image mask(Magick::Geometry(width, height),
                     Magick::Color("transparent"));
  mask.draw(Magick::DrawableRoundRectangle(0, 0, width, height, options.radius,
                                           options.radius));
  options.mask = mask;
}

void roundedCornerMask(Options &options) {
  update_status("Rounding image: Initialized");
  if (options.radius <= 0) {
    update_status("Raduis set to " + to_string(options.radius) +
                  " , Skipping Rounding");
    return;
  }
  thread maskThread(createMask, ref(options));
  maskThread.join();
  update_status("Rounding image: Compositing");
  options.mask.composite(options.rotatedAndMergedImage, Magick::CenterGravity,
                         Magick::InCompositeOp);
  options.roundedImage = options.mask;
  options.finalImage = options.mask;
  update_status("Rounding image: Completed");
}

void getGradientColor(Options &options, int index, bool from = true) {
  Magick::Image image(options.images[index]);
  image.resize(Magick::Geometry(1, 1));
  image.alpha(false);
  Magick::Color pixelColor = image.pixelColor(0, 0);
  if (from) {
    options.gradientFromColor = pixelColor;
  } else {
    options.gradientToColor = pixelColor;
  }
}

void paddingAndGradient(Options &options) {
  if (options.padding <= 0) {
    update_status("Padding set to " + to_string(options.radius) +
                  " , Skipping Padding and Gradients");
    return;
  }
  thread fromColorThread, toColorThread;
  if (options.gradientFromColor.empty()) {
    fromColorThread = thread(getGradientColor, ref(options), 0, true);
  }

  if (options.gradientToColor.empty()) {
    toColorThread = thread(getGradientColor, ref(options), 0, false);
  }
  if (options.gradientFromColor.empty())
    fromColorThread.join();
  if (options.gradientToColor.empty())
    toColorThread.join();

  update_status("Fetched Colors: " + options.gradientFromColor + " & " +
                options.gradientToColor + "\n");
  update_status("Padding and Gradient: Creating Gradient");
  size_t imageWidth = options.finalImage.columns();
  size_t imageHeight = options.finalImage.rows();
  // create and save gradient image
  Magick::Image gradientImage(Magick::Geometry(options.padding + imageHeight,
                                               options.padding + imageWidth),
                              Magick::Color(options.gradientFromColor));

  gradientImage.rotate(-options.gradientAngle);
  update_status("Padding and Gradient: Applying Gradient");
  gradientImage.composite(options.finalImage, Magick::CenterGravity,
                          Magick::OverCompositeOp);
  options.gradientImage = gradientImage;
  options.finalImage = gradientImage;
  update_status("Padding and Gradient: Completed\n");
}

int main(int argc, char *argv[]) {
  Magick::InitializeMagick(*argv); // Initialize the Magick++ library
  Options options;
  parseArguments(argc, argv, options);
  // printParsedArguments(options);
  cout << "Rotating and Merging Images: ";
  rotateAndMergeImages(options, options.images);
  cout << "\nRounding Image: ";
  roundedCornerMask(options);
  cout << "\nPadding and Gradient: ";
  paddingAndGradient(options);
  options.finalImage.write(options.outputFileName + "." + options.format);
  cout << "Output written to " << options.outputFileName << "."
       << options.format << endl;
  return 0;
}
