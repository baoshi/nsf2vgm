# nsf2vgm

Convert NSF music to VGM files

## Usage
### nsf2vgm file.nsf [track no]
Read .nsf file, convert each track into separate .vgm file. 


### nsf2vgm config.json [track no]
Convert .nsf to .vgm with fine control. Refer test/template.json for configuration format.

## About sound track boundary and loop detection
Generally each nsf sound track is an infinite loop. nsf2vgm will try to detect the loop by recording register write operations and find a repeating pattern. But this is not always accurate. User can control the looping finding using .json configuration, specify the following parameters:

### "max_track_length": 120
This is the maximum track length in seconds to decode. Some long sound track requires this value to be increased to find a loop.

### "max_records": 100000
Similar to max_track_length, this specifies maximum register write operations to record.

### "silence_detection": true
To enable/disable silence detection. If the song went silent during playback then we consider it is the end of song.

###  "min_silence": 2
Length of silence to be considered as end of song (in seconds).

### "loop_detection": true
To enable/disable loop detection

### "min_loop_records": 1000
Minimul number of records to be considered a loop. For example, a song may contain repeating patterns like AABAABAAB, if A contains more records than min_loop_records, the program will errorously consider A as the loop region. Increase min_loop_records to overcome this problem.

