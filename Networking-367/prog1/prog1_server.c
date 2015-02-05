#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#define QLEN 6 /* size of request queue */

/*------------------------------------------------------------------------
* Program: server
*
* Purpose: allocate a socket and then repeatedly execute the following:
* (1) wait for the next connection of two clients
* (2) create a game based on type specified in argv
* (3) fork child game process and let users play Connect 4
* (4) go back to step (1)
*
* Syntax: server [ port ] [ game type ]
*
* port - protocol port number to use
*
* Note: The port argument is optional. If no port is specified,
* the server uses the default given by PROTOPORT.
*
* Authors: Jimmy Collins
*
*------------------------------------------------------------------------
*/

// Protos
int player_move_standard(int desired_player_move, char * game_board, int playerNumber);
int check_winner_standard(char * game_board, int player_number);
int player_move_popout(int player_desired_spot, char * game_board, int player_number);
int check_winner_antistack(char * game_board, int player_number);

// Main
int main(int argc, char **argv) {
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold server's address */
	struct sockaddr_in cad; /* structure to hold client's address */
	int sd, player1, player2; /* socket descriptors */
	int port; /* protocol port number */
	int alen; /* length of address */
	char game_board[47]; /* buffer for string the server sends */
	char game_type;
	
	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server server_port game_type\n");
		exit(EXIT_FAILURE);
	}

	if (strcmp("standard", argv[2]) == 0) // standard
	{
		game_type = 'S';
	}
	else if (strcmp(argv[2], "popout") == 0)
	{
		game_type = 'P';
	}
	else if (strcmp(argv[2], "antistack") == 0)
	{
		game_type = 'K';
	}
	else
	{
		printf("Game Type Not Supported! Exit!");
		exit(EXIT_FAILURE);
	}

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */
	sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	port = atoi(argv[1]); /* convert argument to binary */
	if (port > 0) { /* test for illegal value */
		sad.sin_port = htons((u_short)port);
	} else { /* print error message and exit */
		fprintf(stderr,"Error: Bad port number %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Bind a local address to the socket */
	if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	/* Specify size of request queue */
	if (listen(sd, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}

	// PID for the forked child process
	int cpid = 0;	

	/* Main server loop - accept and handle requests */
	while (1) 
	{
		alen = sizeof(cad);

		//Guest 1
		if ( (player1=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}
		char playerNumber;
		playerNumber = '2';
		send(player1, &game_type, 1, 0);
		send(player1, &playerNumber, 1, 0);
		
		//Guest 2
		if ( (player2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}
		send(player2, &game_type, 1, 0);
		

		// Tell system to clean up zombie children
		signal(SIGCHLD, SIG_IGN);

		// Fork a child
		cpid = fork();
		if (cpid <0) {
			perror("fork");
			return;
		}

		// Child does the work -- GAME TIME!!!!
		if (cpid == 0)
		{			
			memset(&game_board, '0', sizeof(game_board)); //board set to all zero's
			int temp_sd;
			temp_sd = 0;
			int active_player;
			active_player = player1; //player one goes first
			int inactive_player;
			inactive_player = player2;
			int player_number;
			player_number = 1;
			char your_turn;
			your_turn = 'Y';
			char wait_turn;
			wait_turn = 'H';
			char invalid_move;
			invalid_move = 'I';
			int go = 1; 
			int win_status = 50;
			char playerMove[2];
			int valid_move;
			char win;
			win = 'W';
			char lose;
			lose = 'L';
			char tie;
			tie = 'T';
			int player_desired_spot;
			//Standard Game			
			if (game_type == 'S')
			{			
				while (go == 1)
				{
					send(active_player, &your_turn, 1, 0); //Send your turn to active player
					send(active_player, &game_board, 42, 0);
					send(inactive_player, &wait_turn, 1, 0); //Send hold to inactive player
					send(inactive_player, &game_board, 42, 0);
					
					//get move from active player
					recv(active_player, &playerMove, 2, 0);
					if (playerMove[0] == 'A' && playerMove[2] == 0)
					{
						//Get Row Number
						player_desired_spot = playerMove[1] - '0';
						//check if move is valid
						valid_move = player_move_standard(player_desired_spot, game_board, player_number);
						while (valid_move == -1)
						{
							send(active_player, &invalid_move, 1, 0);
							recv(active_player, &playerMove, 2, 0);
							player_desired_spot = 0;
							player_desired_spot = playerMove[1] - '0';
							valid_move = player_move_standard(player_desired_spot, game_board, player_number);
						} 
						win_status = check_winner_standard(game_board, player_number);
						if (win_status == 1) //win detected
						{
							send(active_player, &win, 1, 0);
							send(inactive_player, &lose, 1, 0);
							go = 0;
							close(active_player);
							close(inactive_player);
							exit(EXIT_FAILURE); 
						}
						else if (win_status == 2)  //tie detected
						{
							send(active_player, &tie, 1, 0);
							send(active_player, &tie, 1, 0);	
							close(active_player);
							close(inactive_player);
							exit(0); 
						}	
					}
					else if (playerMove[0] == 'P')
					{
						send(active_player, &invalid_move, 1, 0); //invalid move
					}
					else 
					{
						send(active_player, &invalid_move, 1, 0); //invalid move
					}

					//set player number for next round
					if (player_number == 1)
					{
						player_number = 2;
					}
					else
					{
						player_number = 1;
					}
					temp_sd = active_player;
					active_player = inactive_player;
					inactive_player = temp_sd;
				}
			} //end standard game
			else if (game_type == 'P') //start popout
			{
				while (go == 1)
				{
					send(active_player, &your_turn, 1, 0); //Send your turn to active player
					send(active_player, &game_board, 42, 0);
					send(inactive_player, &wait_turn, 1, 0); //Send hold to inactive player
					send(inactive_player, &game_board, 42, 0);
					
					
					while (go == 1) //while active player's turn
					{
						//get move from active player
						recv(active_player, &playerMove, 2, 0);	
						player_desired_spot = playerMove[1] - '0';
						if (playerMove[0] == 'A' && playerMove[2] == 0)
						{
							valid_move = player_move_standard(player_desired_spot, game_board, player_number);
						}
						else if (playerMove[0] == 'P' && playerMove[2] == 0)
						{
							valid_move = player_move_popout(player_desired_spot, game_board, player_number);
						}
						if (valid_move == 1) //yes it was a valid move
						{
							go = 2; //break out of loop
							win_status = check_winner_standard(game_board, player_number);
							if (win_status == 1) //win detected
							{
								send(active_player, &win, 1, 0);
								send(inactive_player, &lose, 1, 0);
								go = 0;
								close(active_player);
								close(inactive_player);
								exit(0); 
							}
							else if (win_status == 2)  //tie detected
							{
								send(active_player, &tie, 1, 0);
								send(active_player, &tie, 1, 0);	
								close(active_player);
								close(inactive_player);
								exit(0); 
							}	
						}
						else
						{
							send(active_player, &invalid_move, 1, 0);
						}
						
					}
					go = 1; //reset go
	
					//set player number for next round
					if (player_number == 1)
					{
						player_number = 2;
					}
					else
					{
						player_number = 1;
					}
					temp_sd = active_player;
					active_player = inactive_player;
					inactive_player = temp_sd;
				}
			}//end popout
			//start antistack
			else if (game_type == 'K')
			{
				while (go == 1)
				{
					send(active_player, &your_turn, 1, 0); //Send your turn to active player
					send(active_player, &game_board, 42, 0);
					send(inactive_player, &wait_turn, 1, 0); //Send hold to inactive player
					send(inactive_player, &game_board, 42, 0);
					
					//get move from active player
					recv(active_player, &playerMove, 2, 0);
					if (playerMove[0] == 'A' && playerMove[2] == 0)
					{
						//Get Row Number
						player_desired_spot = playerMove[1] - '0';
						//check if move is valid
						valid_move = player_move_standard(player_desired_spot, game_board, player_number);
						while (valid_move == -1)
						{
							send(active_player, &invalid_move, 1, 0);
							recv(active_player, &playerMove, 2, 0);
							player_desired_spot = 0;
							player_desired_spot = playerMove[1] - '0';
							valid_move = player_move_standard(player_desired_spot, game_board, player_number);
						} 
						win_status = check_winner_antistack(game_board, player_number);
						if (win_status == 1) //win detected
						{
							send(active_player, &lose, 1, 0);
							send(inactive_player, &win, 1, 0);
							go = 0;
							close(active_player);
							close(inactive_player);
							exit(EXIT_FAILURE); 
						}
						else if (win_status == 2)  //tie detected
						{
							send(active_player, &tie, 1, 0);
							send(active_player, &tie, 1, 0);	
							close(active_player);
							close(inactive_player);
							exit(0); 
						}	
					}
					else if (playerMove[0] == 'P')
					{
						send(active_player, &invalid_move, 1, 0); //invalid move
					}
					else 
					{
						send(active_player, &invalid_move, 1, 0); //invalid move
					}

					//set player number for next round
					if (player_number == 1)
					{
						player_number = 2;
					}
					else
					{
						player_number = 1;
					}
					temp_sd = active_player;
					active_player = inactive_player;
					inactive_player = temp_sd;
				}	
			}
		}
	}	
}


//Checks to see if move was valid
//Take in the the desired spot the player wants their token to go, game board and active player number
//returns if it was successful or not
int player_move_standard(int desired_player_move, char * game_board, int playerNumber)
{
	int max_size;
	max_size = 41;
	int i;
	int current_working_spot;
	current_working_spot = 100;
	if (!(desired_player_move <= 6 && desired_player_move >= 0))
	{
		return -1;
	}
	for (i = desired_player_move; i <= max_size; i += 7)
	{
		if (game_board[i] == '0')
		{
			current_working_spot = i;
		}
	} 
	if (current_working_spot == 100)
	{
		return -1;
	}	 
	else
	{
		game_board[current_working_spot] = (char)(((int)'0')+playerNumber); //converts to char
		return 1;
	}	
}

//checks player move for validity in popout game
//Take in the the desired spot the player wants their token to be popped out, game board and active player number
//returns a status code
int player_move_popout(int player_desired_spot, char * game_board, int player_number)
{
	int value_at_bottom;
	//int converted_player_number;
	int bottom_index;
	//converted_player_number = 0;
	bottom_index = player_desired_spot + 35;
	//converted_player_number = player_number - '0';
	value_at_bottom = game_board[(player_desired_spot + 35)] - '0'; //changes to number
	if (player_number  == value_at_bottom)
	{
		int i;
		i=0;
		for (i=0; i < 29; i += 7)
		{
				game_board[bottom_index - i] = game_board[bottom_index - i -7];
		}
		game_board[player_desired_spot] = '0';
		return 1;
	}
	else
	{
		return -1;
	}
}

//Check to see if their is a winner for standard and popout game types
//Take in the game board and active player number
//returns a status code
int check_winner_standard(char * game_board, int player_number)
{
	int row;
	int col;
	char active_player;
	active_player = (char)(((int)'0') + player_number);
	//Check Vertical Win
	for (col = 0; col <= 6; col ++)
	{
		row = 0;
		for (row = 0; row <= 14; row+=7)
		{		
			if (game_board[row+col] == active_player && game_board[row+col+7] == active_player && game_board[row+col+14] == active_player && game_board[row+col+21] == active_player)
			{
				return 1;			
			} 
		} 
	}
	
	//Check Horizontal Win 	
	for (row=0; row < 42; row+=7)
	{
		col = 0;
		for (col=0; col < 4; col ++)
		{		
			if (game_board[row+col] == active_player && game_board[row+col+1] == active_player && game_board[row+col+2] == active_player && game_board[row+col+3] == active_player)  	
			{
				return 1;				
			}
		}
	}
	
	//check diag win
	for (row=0; row < 4; row ++)
	{
		col = 0;
		for (col = 0; col < 17; col += 7)
		{
			if (game_board[row+col] == active_player && game_board[row+col+8] == active_player && game_board[row+col+16] == active_player && game_board[row+col+24] == active_player)
			{
				return 1;
			}  	
		}
	}
	//check diag win 2
	for (row=21; row < 25; row ++)
	{
		col = 0;
		for (col = 0; col < 18; col += 7)
		{
			if (game_board[row+col] == active_player && game_board[row+col-6] == active_player && game_board[row+col-12] == active_player && game_board[row+col-18] == active_player)
			{
				return 1;
			}  	
		}
	}

	//CHECK FOR TIE
	int tie = 20;
	row = 0;
	for (row = 0; row < 7; row ++)
	{
		if (game_board[row] == '0')
		{
			return -1; //not a tie
		} 
	}	
	//returns 2 for tie
	return 2; 
}

//Check to see if their is a winner for antistack game
//Take in the game board and active player number
//returns a status code
int check_winner_antistack(char * game_board, int player_number)
{
	int row;
	int col;
	char active_player;
	active_player = (char)(((int)'0') + player_number);
	//Check Vertical Win
	for (col = 0; col <= 6; col ++)
	{
		row = 0;
		for (row = 0; row <= 21; row+=7)
		{		
			if (game_board[row+col] == active_player && game_board[row+col+7] == active_player && game_board[row+col+14] == active_player)
			{
				return 1;			
			} 
		} 
	}
	
	//Check Horizontal Win 	
	for (row=0; row < 42; row+=7)
	{
		col = 0;
		for (col=0; col < 5; col ++)
		{		
			if (game_board[row+col] == active_player && game_board[row+col+1] == active_player && game_board[row+col+2] == active_player)  	
			{
				return 1;				
			}
		}
	}
	
	//check diag win
	for (row=0; row < 5; row ++)
	{
		col = 0;
		for (col = 0; col < 22; col += 7)
		{
			if (game_board[row+col] == active_player && game_board[row+col+8] == active_player && game_board[row+col+16] == active_player)
			{
				return 1;
			}  	
		}
	}
	//check diag win 2
	for (row=14; row < 18; row ++)
	{
		col = 0;
		for (col = 0; col < 23; col += 7)
		{
			if (game_board[row+col] == active_player && game_board[row+col-6] == active_player && game_board[row+col-12] == active_player)
			{
				return 1;
			}  	
		}
	}

	//CHECK FOR TIE
	int tie = 20;
	row = 0;
	for (row = 0; row < 7; row ++)
	{
		if (game_board[row] == '0')
		{
			return -1; //not a tie
		} 
	}	
	//returns 2 for tie
	return 2; 
}
