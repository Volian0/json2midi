# json2midi
JSON to MIDI converter for Piano Tiles 2
# How to install
`git clone https://github.com/Volian0/json2midi`  
`cd json2midi`  
`g++ src/pt2.cpp src/main.cpp -std=c++11 -o json2midi`  
# How to update
`cd json2midi`  
`git pull`  
`g++ src/pt2.cpp src/main.cpp -std=c++11 -o json2midi`  
# How to use
There are 2 ways to convert a JSON file
## Automatic
If the JSON file contains bpms and baseBeats, you can just simply pass the filename as parameter  
`json2midi song.json`  
or  
`json2midi song.json auto`  
## Manual
In this mode, you need to specify all required bpms and baseBeats for each part  
`json2midi song.json 120 0.25 122 0.25 124 0.5`  
