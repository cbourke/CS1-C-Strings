#include <stdio.h>
#include <math.h>
#include <string.h>

#include "gameFunctions.h"

initializeBlankString() {}

printWithSpaces() {}

revealGuessedLetter() {}

checkGuess() {}

void startGame(char word[25]) {
  int won = 0;  // Flag to see if the user has won yet
  int numBadGuesses = 0;  // Counter to end the game on a lose condition
  int possibleBadGuesses;  // Total number of bad guesses allowed
  int charRevealed;  // Flag to see if the user guessed a good letter
  char guess;  // The user's guess
  char revealedLetters[25];  // What the user has revealed so far

  // Initializes the guessing array to all underscores
  initializeBlankString(strlen(word), revealedLetters);

  // Gets the total number of chances
  printf(
      "Please enter the total number of incorrect guesses you would like to be "
      "able to make: ");
  scanf("%d", &possibleBadGuesses);
  printWithSpaces(revealedLetters);

  // Runs the game loop until the number of tries are exhausted or the word is
  // found
  while (numBadGuesses < possibleBadGuesses && !won) {
    printf("Enter a letter to guess: ");
    scanf(" %c", &guess);

    // Updates the revealed letters and checks to see if the user won
    charRevealed = revealGuessedLetter(word, revealedLetters, guess);
    won = checkGuess(word, revealedLetters);

    // Increments bad guesses if the last guess was a miss
    if (!charRevealed) {
      numBadGuesses++;
      printf("Sorry, there is no %c\n", guess);
    } else {
      printf("Yes, '%c' is in the solution!\n", guess);
    }

    // Outputs game information to the user
    printWithSpaces(revealedLetters);
  }

  if (won) {
    printf("Congratulations!  You correctly guessed the word %s\n", word);
  } else {
    printf("You've run out of guesses.  The correct word was %s\n", word);
  }
}
