# Zen Flow
This sketch was so beloved by the Beta Dev Kit community that we decided it belongs as a core Blinks experience. Zen Flow is not necessarily a game, although we have seen games emerge from its mesmerizing interactivity.

## Actions
1. Single click spreads the next color to all contiguous Blinks
2. Double click sparkles a random color to all contiguous Blinks
3. Long press changes from emit/spread mode to connect mode in which it displays the color of the ones it is connected to

## Purpose
The goal of this application, besides being particularly meditative, is to show the different inputs and how one might use them in game play. The single click is typically the simplest of inputs and is best used for your basic actions. A double click might be equivalent to a right click on a mouse, a slightly different action type. Long presses lend themselves to mode changes and of course, moving Blinks around is a huge benefit to Blinks games.

## Responsiveness
When a user performs an action, i.e. a press or a rearrangement, the Blinks should signal their awareness of this activity. In this case, we do so with glowing or reducing brightness ever so slightly to say, "hey, I hear ya" If the Blinks don't signal that the input was received, even when the input is not a meaningful one for the game, the Blinks might feel ill-responsive or broken. You don't want people to feel like the Blinks aren't responding to them, so take a look at how we use subtle acknowledgements here.

