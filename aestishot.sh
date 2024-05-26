#!/bin/bash

images=()
parts=()
padding=80
radius=20
angle=9
output="output"
type="png"

textFontSize=50
textXPos=0
textYPos=0
textColor="black"
textFont=$(fc-match -f "%{file}\n" :lang=en | head -n 1)


usage() {
    echo "Usage: $0 [options] <image1> <image2>"
    echo "Options:"
    echo "  -h Prints this useage"
    echo "  -p <padding>     Padding size for the gradient background (default: 80)"
    echo "  -r <radius>      Corner radius for the output image (default: 20)"
    echo "  -a <angle>       Angle to rotate the output image by (default: 9)"
    echo "  -t <png|jpeg|...|tiff>  File Format CAUTION: JPEG might not work very well"
    echo "  -o <file Name>   Name of the output file (default: output.png)"   
    echo "  -gf <color> Speicfy Gradient fromColor color (default: generated from first supplied image)"
    echo "  -gt <color> Speicfy Gradient toColor color (default: generated from last supplied image)"
    echo "  -ga <angle> Speicfy Gradient Angle (default: 180-angle)"
    echo "  -ts <Text_String> Speicfy Text to be added to the image"
    echo "  -ta <Text_Angle> Speicfy Text Angle"
    echo "  -tx <X-Position> Speicfy X-Offset of the Text from Center of image"
    echo "  -ty <Y-Position> Speicfy Y-Offset of the Text from Center of image"
    echo "  -tc <color> Speicfy Text Color"
    echo "  -tf <font> Speicfy Text Font"
    echo "  -tfs <Text_Size> Speicfy Text Size"
    echo "  -lf Lists all available fonts"
    echo "EXAMPLE:"
    echo "$0 -r 25 -p 32 -a 9 -gf \"#ff0000\" -gt \"#00ff00\" -ga 90 -t png -o myoutput images*.png"
    exit 1
}

listFonts(){
    magick -list font | grep Font: | awk '{print $2}'
    exit 1
}

rotateAndMerge() {
    echo "Hang tight this may take some time"
    # Calculate the height of each cropped part
    num_images=${#images[@]}

    # Create temporary files for parts
    for ((i = 0; i < $(( num_images + 2 )); i++)); do
        parts+=( "$(mktemp /tmp/part_"$i.XXXXXX.$type")" )
    done

    echo -n "Rotating and merging | Status: "
    # Crop the parts using the calculated positions
    for ((i = 0; i < num_images; i++)); do
        {
        magick "${images[i]}" -background none -rotate "$angle" -trim +repage "$type:${parts[i]}"
        # Calculate the dimensions of the first image
        read -r image_width image_height posx posy <<< "$(identify -format "%w %h %[fx:page.x] %[fx:page.y]" "${parts[i]}")"
        crop_height=$(( image_height / num_images ))

        position=$((crop_height * i))
        new_posy=$((posy + position))
        magick "${parts[i]}" -crop "${image_width}x${crop_height}+${posx}+${new_posy}" "$type:${parts[i]}"
        } &
    done

    # Wait for all background jobs to finish
    wait

    # Combine the parts vertically and save as "$output".$type
    magick "${parts[@]::${#parts[@]}-2}" -append "$output.$type"

    if [[ "$angle" == -* ]]; then
        # If it starts with "-", replace it with "+"
        angle="${angle/-/}"
    else
        # If it doesn't start with "-", prepend it with "-"
        angle="-${angle}"
    fi

    magick "$output.$type" -background none -rotate "$angle" -trim +repage "$type:$output.$type"
      
    # Remove the "-" from the angle
    angle="${angle/-/}"

    echo "Done"

}

roundImageCorners() {
    if [[ "$radius" -ne 0 ]]; then
        echo -n "Rounding Inner Image | Status: "
        read -r W H <<< "$(identify -format "%w %h" "$output.$type")"
        magick -size "${W}x${H}" xc:none -draw "roundrectangle 0,0,$W,$H,$radius,$radius" "$type:${parts[-2]}"
        magick "$output.$type" -alpha Set "${parts[-2]}" -compose DstIn -composite "$output.$type"
        echo "Done"
    else
        echo "Radius:$radius Skipping Rounding"
    fi
}

function gradientPadding(){
if [[ "$padding" -ne 0 ]]; then
    echo -n "Adding gradient padding "
    if [[ -z "$fromColor" ]]; then
    fromColor=$(magick "${images[0]}" -alpha off -resize 1x1 -format "#%[hex:u]" info:-)
    fi
    if [[ -z "$toColor" ]]; then
    toColor=$(magick "${images[-1]}" -alpha off -resize 1x1 -format "#%[hex:u]" info:-)
    fi

    echo -n "with Colors $fromColor $toColor | Status: "
    read -r W H <<< "$(identify -format "%w %h" "$output.$type")"
    magick -size $(( W + padding ))x$(( H + padding ))  -define gradient:angle="$gradAngle" gradient:"$fromColor-$toColor" "$type:${parts[-1]}"

    composite -gravity center "$output.$type" "${parts[-1]}"  "$output.$type"
    echo "Done"
else
    echo "Padding:$padding Skipping Padding"
fi
}

addText(){
if [[ -n "$text" ]]; then
    echo -n "Adding Text"

    textFontSize=$(( W / ${#text} ))
    if [[ "$textFontSize" -lt 10 ]]; then
        textFontSize=50
    fi
    if [[ -z "$textAngle" ]]; then
        textAngle=$(( 360 - angle ))
    fi 
    echo -n "With Font Size $textFontSize | Status: "
    magick "$output.$type" -fill "$textColor" -font "$textFont" -pointsize "$textFontSize" -gravity center -annotate +"${textAngle}x${textAngle}+$textXPos+$textYPos" "$text" "$type:$output.$type"
    echo "Done"
else
    echo "Text:$text Skipping Text"
fi
}

cleanUp() {
    # Clean up temporary files
    echo -n "Cleaning Up temporary files | Status: "
    rm "${parts[@]}"
    echo "Done"
    echo "Check out the $output.$type"
}

handleInterrupt() {
    echo -e "\nStop Spamming Ctrl+C, I am cleaning up!"
    echo -n "Cleaning Up temporary files | Status: "
    rm  "${parts[@]}" && echo "Done; Have a great restart!"
    exit 1
}

main(){
while [[ $# -gt 0 ]]; do
    case $1 in
       -p)
            shift
            padding=$1
            ;;
        -r)
            shift
            radius=$1
            ;;
        -t)
            shift
            type=$1
            ;;
        -o)
            shift
            output=$1
            ;;
        -gf)
            shift
            fromColor="$1"
            ;;
        -ga)
            shift
            gradAngle="$1"
            ;;
        -gt)
            shift
            toColor="$1"
            ;;
        -a)
            shift
            angle=$1
            ;;
        -ts)
            shift
            text="$1"
            ;;
        -tx)
            shift
            textXPos=$1
            ;;
        -ty)
            shift
            textYPos=$1
            ;;
        -tc)
            shift
            textColor=$1
            ;;
        -tf)
            shift
            textFont=$1
            ;;
        -tfs)
            shift
            textFontSize=$1
            ;;
        -ta)
            shift
            textAngle=$1
            ;;
        -lf)
            listFonts
            ;;
        -h)
            usage
            ;;
        -*)
            echo "Unrecognized option: $1" >&2
            usage
            ;;
        *)
            images+=("$1")
            ;;
    esac
    shift
done

if [[ -z "$gradAngle" ]]; then
gradAngle=$(( 180 - angle ))
fi

if [ ${#images[@]} -eq 0 ]; then
    echo "No images provided."
    usage
fi

rotateAndMerge
roundImageCorners
gradientPadding
addText
cleanUp
}

trap handleInterrupt SIGINT SIGTERM
main "$@"
