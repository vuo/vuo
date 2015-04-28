Fires events when characters, words, and lines are typed. 

   - `window` — If a window is provided, then keyboard buttons are only tracked when the window is active. Otherwise, keyboard buttons are tracked when any window in the composition is active. 
   - `typedLine` — When a line ending (Return key) is typed, fires an event with the line just completed. 
   - `typedWord` — When a word ending is typed, fires an event with the word just completed. A word ending is a whitespace character (Return, Space, or Tab key) that follows a non-whitespace character. 
   - `typedCharacter` — When a character is typed, fires an event with the character just completed. When multiple keystrokes are used to create a single character (such as *Option-e-e* to type "é" on some keyboards), this port only fires for the final keystroke, not the intermediate keystrokes. 
