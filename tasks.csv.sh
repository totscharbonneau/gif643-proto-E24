cd $( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
rm --force tasks.csv
rm -rf out
mkdir out
ls data | while read FILE; do
    echo "data/$FILE;out/$(echo "$FILE" | sed -e 's/.svg/-240.png/g');240" >> tasks.csv
    echo "data/$FILE;out/$(echo "$FILE" | sed -e 's/.svg/-480.png/g');480" >> tasks.csv
    echo "data/$FILE;out/$(echo "$FILE" | sed -e 's/.svg/-960.png/g');960" >> tasks.csv
done