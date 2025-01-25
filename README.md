# RC Project - Master Mind Game

This project implements a simple version of the Master Mind game using a sockets interface in C. It consists of two main components:
- **Game Server (GS)**: Handles game logic and player interactions.
- **Player Application (Player)**: Allows players to interact with the Game Server.

The Game Server and Player Application are designed to operate simultaneously, enabling users to play the game over a network.

---

## Player Commands

Here are the commands supported by the Player Application:

- **Start the game**:  
  ```start <PLID> <max_playtime>```  
  Begins a new game.  
  - `<PLID>`: Player ID (e.g., a unique identifier for the player).  
  - `<max_playtime>`: Maximum time for the game (in seconds).  

- **Make a guess**:  
  ```try <C1> <C2> <C3> <C4>```  
  Attempt to guess the secret code.  
  - `<C1>, <C2>, <C3>, <C4>`: Colors chosen for the guess.  
  - The valid colours are: `red (R), green (G), blue (B), yellow
(Y), orange (O) and purple (P)`

- **Show current tries**:  
  ```show_trials``` or ```st```  
  Displays all the guesses made so far and the feedback for each guess.  

- **Show the scoreboard**:  
  ```scoreboard``` or ```sb```  
  Displays the game's scoreboard with player rankings.  

- **Quit the current game**:  
  ```quit```  
  Ends the current game without completing it.  

- **Exit the application**:  
  ```exit```  
  Closes the Player Application.  

- **Open the game in debug mode**:  
  ```debug <PLID> <max_playtime> <C1> <C2> <C3> <C4>```  
  Starts a new game in debug mode, using the provided secret code.  

---

## Compiling and Running

### Compiling the Code
Use the following command to compile the project:  
```bash
make
```

### Running the Player Application

Run the Player Application with the following command:

```bash
./player <-n GSIP> <-p GSport>
```

- **GSIP**: The IP address of the machine where the Game Server is running. If omitted, it defaults to the local machine (localhost).
- **GSport**: The well-known port (TCP and UDP) where the Game Server accepts requests. If omitted, it defaults to 58015.

### Running the Game Server

Run the Game Server with the following command:

```bash
./GS <-p GSport> <-v>
```

- **GSport**: The well-known port where the Game Server listens for requests. If omitted, it defaults to 58015.
- **-v**: Enables verbose mode, providing additional debugging information.


