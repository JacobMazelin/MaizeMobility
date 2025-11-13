
//Speaker start
const int speakerPin = 8; //Pin connector for the speaker (could change)
const int frequency = 1000; // sound in Hz

void setup() {
    tone(speakerPin, frequency);  //speaker continues to play
}



//Speaker stop
noTone(speakerPin);  //stops sound
