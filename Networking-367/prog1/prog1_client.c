#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/*------------------------------------------------------------------------
* Program: client
*
* Purpose: allocate a socket, connect to a server, and print all output
*
* Syntax: client [ host [port] ]
*
* host - name of a computer on which server is executing
* port - protocol port number server is using
*
* Note: Both arguments are optional. If no host name is specified,
* the client uses "localhost"; if no protocol port is
* specified, the client uses the default given by PROTOPORT.
*
* Authors: Jimmy Collins
*
*------------------------------------------------------------------------
*/

//Prototypes
void print_board(char * game_board); 
void game_main (char * game_board, int sd);


main( int argc, char **argv) {
	struct hostent *ptrh; 	/* pointer to a host table entry */
	struct protoent *ptrp; 	/* pointer to a protocol table entry */
	struct sockaddr_in sad; 	/* structure to hold an IP address */
	int sd; 					/* socket descriptor */
	int port; 				/* protocol port number */
	char *host; 				/* pointer to host name */
	int n; 					/* number of characters read */
	char severResponse[1]; 	/* buffer for data from the server */
	char game_board[47];

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./client server_address server_port\n");
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[2]); /* convert to binary */
	if (port > 0) /* test for legal value */
	sad.sin_port = htons((u_short)port);
	else {
		fprintf(stderr,"Error: bad port number %s\n",argv[2]);
		exit(EXIT_FAILURE);
	}

	host = argv[1]; /* if host argument specified */

	/* Convert host name to equivalent IP address and copy to sad. */
	ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		fprintf(stderr,"Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	/* Map TCP transport protocol name to protocol number. */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Connect the socket to the specified server. */
	if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"connect failed\n");
		exit(EXIT_FAILURE);
	}

	char guessbuf[100];
	unsigned int guess[1];

	//Receive Game Type and Tell User Type
	char gameType;
	recv(sd, &gameType, sizeof(gameType), 0);
	if (gameType == 'S')
	{
		printf("Game Type is Standard\n");
	}
	else if (gameType == 'P')
	{
		printf("Game Type is Popout\n");
	}
	else if (gameType == 'K')
	{
		printf("Game Type is Antistack\n");  
	}
	else 
	{
		printf("Not Support Game Type! ERROR");  
		exit(EXIT_FAILURE);
	}

	//Get next char from server
	char nextServerResponse;
	recv(sd, &nextServerResponse, sizeof(nextServerResponse), 0);
	if (nextServerResponse == '2') //player 1
	{
		printf("Hi Player One! We're waiting for player two to connect. Game will begin soon!\n");
		game_main(game_board, sd);
	}
	else if (nextServerResponse == 'H') //player 2
	{
		//Greet Player 2
		printf("Hi Player Two! Player One will go first!\n");
		//receive and print game board
		recv(sd, &game_board, 42, MSG_WAITALL);
		print_board(game_board);
		printf("Please Wait For Your Turn\n");
		game_main(game_board, sd);
	} 
	// Game finished, clean up
	close(sd);
	exit(EXIT_SUCCESS);
} //end of Main

//Main Game Logic
void game_main (char * game_board, int sd)
{
		char PlayerMove[100];
		int go = 1;
		char playerStatus;
		while (go == 1)
		{
			//Wait for Player's Turn
			recv(sd, &playerStatus, 1, 0);
			if (playerStatus == 'Y')
			{
				//Player's Turn
				printf("\nIt's your Turn!\n");
				//Get Game Board
				recv(sd, game_board, 42, MSG_WAITALL);
				//print Game Board
				print_board(game_board);
				//Get Move From User
				printf("\nPlease Enter Your Move: ");
				fgets(PlayerMove, 100, stdin);
				send(sd, &PlayerMove, 2, 0);
				//receive response from server
				recv(sd, &playerStatus, 1, 0);
				while (playerStatus == 'I')
				{
					printf("Invalid move or bad syntax. Please try again\n");
					printf("\nPlease Enter Your Move: ");
					fgets(PlayerMove, 100, stdin);
					send(sd, &PlayerMove, 2, 0); //Send new move to server
					recv(sd, &playerStatus, 1, 0);
				}
				if (playerStatus == 'H')
				{
					//Wait for other player move
					printf("\nPlease wait for your turn\n");
					recv(sd, game_board, 42, MSG_WAITALL);
					print_board(game_board);	
				}
				else if (playerStatus == 'W')
				{
					printf("Congrats! You Win the Game!\n");
					go = 0;
					break;
				}
				else if (playerStatus == 'L')
				{
					printf("Sorry, you lost the game. Better Luck Next Time!\n");
					go = 0;
					break;
				}
				else if (playerStatus == 'T')
				{
					printf("It's a tie! Good job, but next time do better!:) \n");
					go = 0;
					break;
				}
			}//handle case when it wasn't your turn and other player just one
			else if (playerStatus == 'W')
			{
				printf("Congrats! You Win the Game!\n");
				go = 0;
				break;
			}
			else if (playerStatus == 'L')
			{
				printf("Sorry, you lost the game. Better Luck Next Time!\n");
				go = 0;
				break;
			}
			else if (playerStatus == 'T')
			{
				printf("It's a Tie! Good job, but next time do better! :)\n");
				go = 0;
				break;
			}
		} 
}


//Print Game Board
//In : Game_board
//Return: none
//Output: Display passed in game board
void print_board(char * game_board)
{
	int i;
	for (i = 0; i < 42; i++)
	{
		if ( i % 7 == 0 && i != 0)
		{
			printf("\n");
		}
		printf(" %c ", game_board[i]);
		fflush(stdout);
		sync();
	}	
	printf("\n");
	printf("---------------------\n");
	printf(" 0  1  2  3  4  5  6\n");
}



