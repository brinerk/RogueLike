#include<raw.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<termios.h>
#include<math.h>
#include<time.h>
#include<sys/ioctl.h>

//ROOM DEFINES
#define MAX_WIDTH 32
#define MAX_HEIGHT 22
#define MIN_WIDTH 4
#define MIN_HEIGHT 4

#define MAX_INV 6

/* TODO
 * more interesting generation
 * less overlapping tunnels
 * branching tunnels
 * snake-y tunnels
 * implement xp
 * enemy movement
 * spawn apple - pickup-able items
 * randomized items 
 * inventory system
 * inventory screen
 * equipment
 * spawn lamps, torches.
 */

int **map, **seenTiles;

int col, row;

struct game {
	int inventoryScreen, inputActive;
};

enum map_codes {
	WALL = 0,
	FLOOR = 1,
	SPIDER = 2,
	APPLE = 3
};

struct item {
	int id, weight, pos_x, pos_y, active;
};

typedef struct player {
	int pos_x, pos_y, max_hp, hp, attack, xp;
} Player;


struct enemy {
	int pos_x, pos_y, type, hp, attack, seen;
};

struct coord {
	int x, y;
};

struct room {
	int x, y, w, h, size;
};

struct asciinum {
	char *castChar;
	int len;
};

struct enemy *enemies;

struct item *items;

struct room *rooms = 0;

struct coord *validTiles, *tunnelTiles;

int validTilesSize, index = 0;

int randNum(int lb, int ub) {
	return ((rand()*time(0))%(ub-lb))+lb;
}

struct asciinum asciiInt(int n) {

	struct asciinum num;

	int c = n;
	int len = 0;

	while(c>0) {
		c/=10;
		len++;
	}

	num.len = len;

	static char *castChar;
	castChar = realloc(castChar,len*sizeof(char));
	for(int l = 0; l < len; l++) {
		castChar[len - l - 1] = (n%10) + 48;
		n = floor(n/10);
	}

	num.castChar = castChar;

	return num;
}

void startScreen(void) {
	struct buffer buf = ABUF_INIT;
	bufferAppend(&buf,"\033[2J", 4);
	bufferAppend(&buf,"\033[?25l",6);
	bufferAppend(&buf,"\033[39;40m",8);
	bufferAppend(&buf,"\033[H",3);
	bufferAppend(&buf,"Press w, a, s, or d to start.", 29);
	write(STDOUT_FILENO,buf.b,buf.len);
	bufferFree(&buf);
}

void endScreen(void) {
	int centerc = floor((col/2)-37);
	int centerr = floor((row/2)-19);
	struct buffer buf = ABUF_INIT;
	bufferAppend(&buf,"\033[?25l",6);
	bufferAppend(&buf,"\033[39;40m",8);
	bufferAppend(&buf,"\033[H",3);
	bufferAppend(&buf, "\033[31;40m", 8);
	bufferAppend(&buf,"╔══════════════════════════════════╗",144);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║                                  ║",42);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║     ██░ ██░  ████░  ██░ ██░      ║",90);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║     ██░ ██░ ██░ ██░ ██░ ██░      ║",90);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║     ██░ ██░ ██░ ██░ ██░ ██░      ║",90);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║     ██░ ██░ ██░ ██░ ██░ ██░      ║",90);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║      ████░  ██░ ██░ ██░ ██░      ║",96);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║       ██░   ██░ ██░ ██░ ██░      ║",81);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║       ██░    ████░   ████░       ║",81);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║                                  ║",42);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║  █████░  ██████░ ██████░ █████░  ║",118);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║  ██░ ██░   ██░   ██░     ██░ ██░ ║",108);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║  ██░ ██░   ██░   ██░     ██░ ██░ ║",108);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║  ██░ ██░   ██░   █████░  ██░ ██░ ║",108);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║  ██░ ██░   ██░   ██░     ██░ ██░ ║",108);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║  ██░ ██░   ██░   ██░     ██░ ██░ ║",108);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║  █████░  ██████░ ██████░ █████░  ║",118);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"║                                  ║",8);
	bufferAppend(&buf, "\r\n", 2);
	bufferAppend(&buf,"╚══════════════════════════════════╝",144);
	write(STDOUT_FILENO,buf.b,buf.len);
	bufferFree(&buf);
	exit(0);
}

int playerInv[MAX_INV] = {0};

void inventoryScreen(int *playerInv) {
	struct buffer buf = ABUF_INIT;
	bufferAppend(&buf,"\033[?25l",6);
	bufferAppend(&buf,"\033[39;40m",8);
	bufferAppend(&buf,"\033[H",3);
	bufferAppend(&buf,"INVENTORY", 9);
	for(int i = 0; i<MAX_INV; i++) {
		if(i>row-1){
			break;
		}
		switch(playerInv[i]) {
			case(0): break;
			case(APPLE): {
				bufferAppend(&buf, "\r\n", 2);
				bufferAppend(&buf, "     [", 6);
				struct asciinum ln = asciiInt(i+1);
				bufferAppend(&buf, ln.castChar, ln.len);
				bufferAppend(&buf, "] ", 2);
				bufferAppend(&buf, "Apple", 5);
				break;
			}
		}
	}
	write(STDOUT_FILENO,buf.b,buf.len);
	bufferFree(&buf);
}

void drawScreen(int map_width, int map_height, int row, int col, int sc_row, int sc_col, Player player, int enemiesSize) {

	char buff[32];
	struct buffer buf = ABUF_INIT;
	bufferAppend(&buf,"\033[?25l",6);
	bufferAppend(&buf,"\033[39;40m",8);
	bufferAppend(&buf,"\033[H",3);


	for(int i=0;i<row;i++) {
		for(int j=0;j<col;j++) {
			int y = sc_row + i;
			int x = sc_col + j;
			if(x == player.pos_x && y == player.pos_y) {
				bufferAppend(&buf, "\033[32;40m@\033[39;40m", 17);
			//enemy
			} else if(map[y][x] == SPIDER && seenTiles[y][x] == 1) {
				bufferAppend(&buf, "\033[31;40mӜ\033[39;40m", 18);
			//enemy seen
			} else if(map[y][x] == SPIDER && seenTiles[y][x] == 2) {
				bufferAppend(&buf, "\033[47;40m░\033[39;40m", 19);
			//apple
			} else if(map[y][x] == APPLE && seenTiles[y][x] == 1) {
				bufferAppend(&buf, "\033[32;40ma\033[39;40m", 18);
			//apple seen
			} else if(map[y][x] == APPLE && seenTiles[y][x] == 2) {
				bufferAppend(&buf, "\033[47;40m░\033[39;40m", 19);
			//bright floor
			} else if(map[y][x] == 1 && seenTiles[y][x] == 1) {
				bufferAppend(&buf, "\033[37;47m \033[39;40m", 17);
			//dim floor
			} else if(map[y][x] == 1 && seenTiles[y][x] == 2) {
				bufferAppend(&buf, "\033[47;40m░\033[39;40m", 19);
			//bright wall
			} else if(map[y][x] == 0 && seenTiles[y][x] == 1) {
				bufferAppend(&buf, "\033[0;38;5;255m█\033[39;40m", 24);
			//dim wall
			} else if(map[y][x] == 0 && seenTiles[y][x] == 2) {
				bufferAppend(&buf, "\033[47;100m█\033[39;40m", 20);
			//light dim wall
			} else if(map[y][x] == 0 && seenTiles[y][x] == 3) {
				bufferAppend(&buf, "\033[90;40m▒\033[39;40m", 19);
			//light dim floor
			} else if(map[y][x] == 1 && seenTiles[y][x] == 3) {
				bufferAppend(&buf, "\033[90;40m░\033[39;40m", 19);
			//nothing
			} else {
				bufferAppend(&buf, " ", 1);
			}
		}
		if(i<row-1) {
			bufferAppend(&buf, "\r\n", 2);
		}
	}

	//probably pretty bad
	//make a status bar func
	bufferAppend(&buf, "HP: ", 4);
	struct asciinum hp = asciiInt(player.hp);
	bufferAppend(&buf, hp.castChar, hp.len); 
	bufferAppend(&buf, "/", 1); 
	struct asciinum max_hp = asciiInt(player.max_hp);
	bufferAppend(&buf, max_hp.castChar, max_hp.len); 
	bufferAppend(&buf, " XP: ", 5);

	//uncommenting this overflows the buffer and causes the screen to scroll, fix
	/*for(int therest = 4+4+hp.len+max_hp.len; therest<col;therest++) {
		bufferAppend(&buf, " ", 1); 
	}*/

	write(STDOUT_FILENO,buf.b,buf.len);
	bufferFree(&buf);
	
}

void genTunnels(int max_rooms) {
	int start_x, final_x, start_y, final_y, offset = 0;

	for(int i = 0; i<max_rooms; i++) {
		struct coord Tile = validTiles[randNum(offset,offset+rooms[i].size)];
		offset += rooms[i].size;
		tunnelTiles[i] = Tile;
	}

	for (int p = 0; p<max_rooms-1;p++) {
		struct coord Tile1 = tunnelTiles[p];
		struct coord Tile2 = tunnelTiles[p+1];

		if (Tile1.x>Tile2.x) {
			start_x = Tile2.x;
			final_x = Tile1.x;

		} else {
			start_x = Tile1.x;
			final_x = Tile2.x;
		}
		if (Tile1.y>Tile2.y) {
			start_y = Tile2.y;
			final_y = Tile1.y;

		} else {
			start_y = Tile1.y;
			final_y = Tile2.y;
		}


		if((Tile1.x<Tile2.x && Tile1.y<Tile2.y) || (Tile1.x>Tile2.x && Tile1.y>Tile2.y)) {

			for (int x = start_x; x<final_x; x++) {
				map[start_y][x] = 1;
			}

			for (int y = start_y; y<final_y; y++) {
				map[y][final_x] = 1;
			}
		} else {

			for (int x = start_x; x<final_x; x++) {
				map[start_y][x] = 1;
			}

			for (int y = start_y; y<final_y; y++) {
				map[y][start_x] = 1;
			}
		}
	}
}

void genRooms(int max_rooms) {
	
	for(int r=0;r<max_rooms; r++) {
		int w = rooms[r].w;
		int x = rooms[r].x;
		int h = rooms[r].h;
		int y = rooms[r].y;

		struct coord tile;

		for(int i=0;i<h;i++) {
			for(int j=0;j<w;j++){
				map[y+i][x+j] = 1;
				tile.x = x+j;
				tile.y = y+i;
				validTiles[index] = tile;
				index++;
			}
		}
	}
}

//could probably combine this code with genItems
void genEnemies(int max, int type, int size) {

	enemies=malloc(max*sizeof(struct enemy));
	for(int s = 0; s<max;s++){
		struct coord randomSpider = validTiles[randNum(1,size)];
		struct enemy spider;
		spider.hp = 4;
		spider.type = SPIDER;
		spider.attack = 1;
		spider.pos_x=randomSpider.x;
		spider.pos_y=randomSpider.y;
		enemies[s]=spider;
	}

	for(int e = 0; e<max; e++) {
		if(enemies[e].type == SPIDER) {
			map[enemies[e].pos_y][enemies[e].pos_x] = SPIDER;
		}
	}
}

void genItems(int max, int type, int size) {

	items=malloc(max*sizeof(struct item));
	for(int s = 0; s<max;s++) {
		struct coord randomTile = validTiles[randNum(1,size)];
		struct item apple;
		apple.id = APPLE;
		apple.active = 1;
		apple.weight = 1;
		apple.pos_x=randomTile.x;
		apple.pos_y=randomTile.y;
		items[s]=apple;
	}

	for(int e = 0; e<max; e++) {
		if(items[e].id == APPLE) {
			map[items[e].pos_y][items[e].pos_x] = APPLE;
		}
	}
}

//slow
int checkSightCircle(int x, int y, int srow, int scol, int dir) {


	for(int i = 0; i<row;i++) {
		for(int j = 0; j<col;j++) {
			int p=i+srow;
			int z=j+scol;

			if(seenTiles[p][z] == 1) {
				seenTiles[p][z] = 2;
			} else if(seenTiles[p][z] == 2) {
				seenTiles[p][z] = 2;
			} else if(seenTiles[p][z] == 3) {
				seenTiles[p][z] = 2;
			} else {
				seenTiles[p][z] = 0;
			}
		}
	}

	int radius = 8;
	float angle = 0;
	int slices = 60;
	int light_dim = 5;

	int final_x, final_y;

	for(int i = 0; i<slices; i++) {
		final_x = x + round(radius*cos(angle));
		final_y = y + round(radius*sin(angle));
		float t = 0;

		//KEY FOR SEEN TILES
		//1 = BRIGHTEST
		//2 = SEEN ALREADY
		//3 = DIM
		
		while(t < 1) {
			int nearest_x = round(x+(final_x-x)*t);
			int nearest_y = round(y+(final_y-y)*t);
			if(map[nearest_y][nearest_x] == 0) {
				if(sqrt((final_x-nearest_x)*(final_x-nearest_x)+(final_y-nearest_y)*(final_y-nearest_y)) < light_dim ) {
					seenTiles[nearest_y][nearest_x] = 3;
				} else {
					seenTiles[nearest_y][nearest_x] = 1;
				}
				break;
			} else if(map[nearest_y][nearest_x] == 1) {
				if(sqrt((final_x-nearest_x)*(final_x-nearest_x)+(final_y-nearest_y)*(final_y-nearest_y)) < light_dim ) {
					seenTiles[nearest_y][nearest_x] = 3;
				} else {
					seenTiles[nearest_y][nearest_x] = 1;
				}
			} else if(map[nearest_y][nearest_x] == SPIDER || map[nearest_y][nearest_x] == APPLE) {
				seenTiles[nearest_y][nearest_x] = 1;
			}
			t += 0.01;
		}
		angle += (2*3.1415)/slices;
	}
}


int sc_col, sc_row;

int main(void) {

	struct game gameinfo;
	gameinfo.inventoryScreen=0;
	gameinfo.inputActive=1;

	struct winsize ws = getWindowSize();
	col = ws.ws_col;
	row = ws.ws_row;

	//MAKE ROOM FOR BAR AT BOTTOM
	row = row-1;

	sc_col = col;
	sc_row = row;

	enableRawMode();

	char c;

	Player player = player;
	player.max_hp = 10;
	player.hp = 10;
	player.attack = 2;


	//MAP STUFF
	int map_height = 100;
	int map_width = 300;

	//INTIALIZE MAP ARRAY
	map=(int**)malloc(map_height*sizeof(int*));
	for(int m=0;m<map_height;m++) {
		map[m]=(int*)malloc(map_width*sizeof(int));
	}

	//INITIALIZE SEEN TILES
	seenTiles=(int**)malloc(map_height*sizeof(int*));
	for(int m=0;m<map_height;m++) {
		seenTiles[m]=(int*)malloc(map_width*sizeof(int));
	}

	int r = 0;
	int max_rooms = 30;

	rooms=malloc(max_rooms*sizeof(struct room));
	tunnelTiles=malloc(max_rooms*sizeof(struct coord));

	//MAKE SOME ROOMS
	while(r<max_rooms) {
		int width=randNum(MIN_WIDTH,MAX_WIDTH);
		int height=randNum(MIN_HEIGHT,MAX_HEIGHT);
		int x=randNum(1,map_width-1);
		int y=randNum(2,map_height-1);
		
		if(x+width>map_width-1 || y+height>map_height-1) {
			continue;
		} else {
			struct room Room;
			Room.x = x;
			Room.y = y;
			Room.h = height;
			Room.w = width;
			Room.size = width*height;

			rooms[r] = Room;

			r++;
		}
	}

	int validTilesSize = 0;

	for(int h = 0; h < max_rooms; h++) {
		validTilesSize += rooms[h].size;
	}

	validTiles = malloc(validTilesSize*sizeof(struct coord));

	genRooms(max_rooms);
	genTunnels(max_rooms);


	//player starting pos
	int randomStart = randNum(1,validTilesSize);

	player.pos_x = validTiles[randomStart].x;
	player.pos_y = validTiles[randomStart].y;

	int spiders = 40;
	genEnemies(spiders, SPIDER, validTilesSize);

	int apples = 50;
	genItems(apples, APPLE, validTilesSize);

	//clean up memory we aren't using anymore
	free(validTiles);
	free(tunnelTiles);
	free(rooms);


	startScreen();

	int dir;

	//main loop
	while(1) {

		if(player.hp <= 0) {
			endScreen();
		}

		//handle input
		if (read(STDIN_FILENO, &c,1) == 1) {
			switch(c) {
				case('q'): exit(0);
				case('a'): {
					if(gameinfo.inputActive == 1) {
						if(map[player.pos_y][player.pos_x-1] == 1 || map[player.pos_y][player.pos_x-1] == APPLE) {
							player.pos_x--; 
						} else if(map[player.pos_y][player.pos_x-1] == SPIDER) {
							for(int e = 0; e<spiders; e++) {
								if(enemies[e].pos_x == player.pos_x-1 && enemies[e].pos_y == player.pos_y) {
									enemies[e].hp-=player.attack;
									player.hp-=enemies[e].attack;
								}
							}
						}
					}
					break;
				}
				case('w'): {
					if(gameinfo.inputActive == 1) {
						if(map[player.pos_y-1][player.pos_x] == 1 || map[player.pos_y-1][player.pos_x] == APPLE) {
							player.pos_y--; 
						} else if(map[player.pos_y-1][player.pos_x] == SPIDER) {
							for(int e = 0; e<spiders; e++) {
								if(enemies[e].pos_x == player.pos_x && enemies[e].pos_y == player.pos_y-1) {
									enemies[e].hp-=player.attack;
									player.hp-=enemies[e].attack;
								}
							}
						}
					}
					break;
				}
				case('d'): {
					if(gameinfo.inputActive == 1) {
						if(map[player.pos_y][player.pos_x+1] == 1 || map[player.pos_y][player.pos_x+1] == APPLE) {
							player.pos_x++; 
						} else if(map[player.pos_y][player.pos_x+1] == SPIDER) {
							for(int e = 0; e<spiders; e++) {
								if(enemies[e].pos_x == player.pos_x+1 && enemies[e].pos_y == player.pos_y) {
									enemies[e].hp-=player.attack;
									player.hp-=enemies[e].attack;
								}
							}
						}
					}
					break;
				}
				case('s'): {
					if(gameinfo.inputActive == 1) {
						if(map[player.pos_y+1][player.pos_x] == 1 || map[player.pos_y+1][player.pos_x] == APPLE) {
							player.pos_y++; 
						} else if(map[player.pos_y+1][player.pos_x] == SPIDER) {
							for(int e = 0; e<spiders; e++) {
								if(enemies[e].pos_x == player.pos_x && enemies[e].pos_y == player.pos_y+1) {
									enemies[e].hp-=player.attack;
									player.hp-=enemies[e].attack;
								}
							}
						}
					}
					break;
				}
				case('g'): {
					if(gameinfo.inputActive == 1) {
						for(int v = 0; v<apples; v++) {
							if(items[v].pos_x == player.pos_x && items[v].pos_y == player.pos_y) {
								for(int m = 0; m<MAX_INV;m++) {
									if(playerInv[m]==0) {
										playerInv[m] = APPLE;
										items[v].active = 0;
										break;
									}
								}
							}
						}
					}
					break;
				}
				case('i'): {
					if(gameinfo.inventoryScreen == 0) {
						gameinfo.inventoryScreen = 1;
						gameinfo.inputActive = 0;
					} else {
						gameinfo.inventoryScreen = 0;
						gameinfo.inputActive = 1;
					}
					break;
				}

				case('p'): player.hp--;
					break;
			}

			//UPDATE ENEMIES ON THE MAP
			//MOVE THIS TO ITS OWN FUNCTION
			for(int en = 0; en<spiders; en++) {
				if(enemies[en].hp <= 0) {
					//TERRIBLE, MAKE ACTIVE FLAG AND DEACTIVATE
					map[enemies[en].pos_y][enemies[en].pos_x] = 1;
				}
			}
			
			//UPDATE ITEMS ON MAP
			for(int en = 0; en<apples; en++) {
				if(items[en].active == 0) {
					map[items[en].pos_y][items[en].pos_x] = 1;
				}
			}

			//REDRAW SCREEN IF PLAYER IS OFF
			if(player.pos_x > sc_col-1) {
				sc_col = player.pos_x-floor(col/2);
			}
			if(player.pos_x < sc_col+1) {
				sc_col = player.pos_x-floor(col/2);
			}
			if(player.pos_y > sc_row-1) {
				sc_row = player.pos_y-floor(row/2);
			}
			if(player.pos_y < sc_row+1) {
				sc_row = player.pos_y-floor(row/2);
			}
			if(sc_row<0) {
				sc_row = 0;
			}
			if(sc_col<0) {
				sc_col = 0;
			}
			if(sc_row>map_height-row-1) {
				sc_row = map_height-row-1;
			}
			if(sc_col>map_width-col) {
				sc_col = map_width-col-1;
			}

			checkSightCircle(player.pos_x, player.pos_y, sc_row, sc_col, dir);

			if(gameinfo.inventoryScreen == 0) {
				drawScreen(map_width, map_height, row, col, sc_row, sc_col, player, spiders);
			} else {
				inventoryScreen(playerInv);
			}
		}
	}

	atProgExit();

	return 0;
}
