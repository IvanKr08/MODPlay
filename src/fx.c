#include "fx.h"

void processTickFX(uint8 channel) {
    Channel* chan = getChannel(channel);
    //Note newNote = chan->lastEffect;

    if (chan->effect == 0xA) {
        if (chan->effectArg > 0x0F) {
            //printFormat("UP VOLUME %02X, FROM %i", 2, (newNote.effectArg >> 4), chan->volume);
            if ((chan->volume + (chan->effectArg >> 4)) <= 64) chan->volume += (chan->effectArg >> 4);
            //printFormat(" TO %i\n", 1, chan->volume);
        }
        else {
            //printFormat("DOWN VOLUME %02X, FROM %i", 2, (newNote.effectArg), chan->volume);
            if ((uint8)(chan->volume - chan->effectArg) <= 64) chan->volume -= chan->effectArg;
            //printFormat(" TO %i\n", 1, chan->volume);
        }
    }
    //if (chan->volume > 64) printFormat("VOLUME ERROR %02X!, %i\n", 2, newNote.effectArg, chan->volume);
}

void processRowFX(uint8 channel) {
    Channel* chan = getChannel(channel);
    switch (chan->effect) {
    //Arpeggio
    case 0x0: {

        break;
    }
    //Portamento up
    case 0x1: {

        break;
    }
    //Portamento down
    case 0x2: {

        break;
    }
    //Tone portamento
    case 0x3: {

        break;
    }
    //Vibrato
    case 0x4: {

        break;
    }
    //Volume slide + Tone portamento
    case 0x5: {

        break;
    }
    //Volume slide + Vibrato
    case 0x6: {

        break;
    }
    //Tremolo
    case 0x7: {

        break;
    }
    //Set panning (Not used)
    case 0x8: {

        break;
    }
    //Set offset
    case 0x9: {
        if (getNote(chan->channelID).note == 0) break;

        if (chan->effectArg != 0) chan->progress = MUL64K(MUL256(chan->effectArg));
        else chan->progress = MUL64K(MUL256(chan->lastOffset));
        break;
    }
    //Volume slide
    case 0xA: {

        break;
    }
    //Position jump
    case 0xB: {

        break;
    }
    //Set volume
    case 0xC: {
        chan->volume = (chan->effectArg > 64) ? 64 : chan->effectArg;
        break;
    }
    //Pattern break
    case 0xD: {

        break;
    }
    //===Extended effects
    case 0xE: {

        break;
    }
    //Set tempo/speed
    case 0xF: {
        if (chan->effectArg == 0) break;

        if (chan->effectArg < 32) {
            song.tpr = chan->effectArg;
        }
        else {
            song.tempo = chan->effectArg;
            song.tps   = MUL2(song.tempo) / 5;
            song.bpt   = A_SR / song.tps;
        }
        break;
    }
    }
}