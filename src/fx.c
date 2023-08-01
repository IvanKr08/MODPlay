#include "fx.h"

void processTickFX(uint8 channel) {
    
}

void processRowFX(uint8 channel) {
    Note newNote = getNote(channel);
    Channel *chan = getChannel(channel);

    switch (newNote.effect) {
    case 0x0: {

        break;
    }
    case 0x1: {

        break;
    }
    case 0x2: {

        break;
    }
    case 0x3: {

        break;
    }
    case 0x4: {

        break;
    }
    case 0x5: {

        break;
    }
    case 0x6: {

        break;
    }
    case 0x7: {

        break;
    }
    case 0x8: {

        break;
    }
    case 0x9: {
        if (newNote.effectArg != 0) chan->progress = MUL64K(MUL256(newNote.effectArg));
        else chan->progress = MUL64K(MUL256(chan->lastOffset));
        break;
    }
    case 0xA: {

        break;
    }
    case 0xB: {

        break;
    }
    case 0xC: {
        chan->volume = (newNote.effectArg > 64) ? 64 : newNote.effectArg;
        break;
    }
    case 0xD: {

        break;
    }
    case 0xE: {

        break;
    }
    case 0xF: {
        if (newNote.effectArg < 32) {
            song.tpr = newNote.effectArg;
        }
        else song.tempo = newNote.effectArg;
        break;
    }
    }
}