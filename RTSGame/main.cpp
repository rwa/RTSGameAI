#include "main.h"
#include "drawmap.h"
#include "astar.h"
#include "tile.h"
#include "unit.h"
#include "player.h"
#include "buildfactory.h"

#include <random>

const int tilesize = 25;
const int playerlimit = 15;

// timing intervals
const int resourceMineInterval = 500;
const int unitSpawnInterval = 10000;
const int unitMoveInterval = 75;
const int aiActInterval = 300;

// board size
int maxr;
int maxc;

auto gen = std::mt19937{ std::random_device{}() };

bool handle_keydown(SDL_Keycode& key,
                    std::vector<player*>& players,
                    unit* currentunit,
                    std::list<unit*>& units,
                    std::vector<std::vector<tile*>>& tiles,
                    std::list<tile*>& factories,
                    SDL_Surface* winSurface, SDL_Window* window)
{
	switch (key)
	{
	case(SDLK_ESCAPE):
                std::cout << "escape key" << std::endl;
		return false;
	case(SDLK_r):
		for (int i = 0; i < players.size(); i++)
		{
			std::cout << "Player " << i << " has " << players[i]->resources_ << " resources." << std::endl;
		}
		break;
	case(SDLK_f):
	{
		int mousex;
		int mousey;
		SDL_GetMouseState(&mousex, &mousey);
		int row = mousey / tilesize;
		int column = mousex / tilesize;
		if(currentunit != NULL) currentunit->buildFactory(units, tiles, factories, 1);
		currentunit = NULL;
		break;
	}
	case(SDLK_b):
	{
		int mousex;
		int mousey;
		SDL_GetMouseState(&mousex, &mousey);
		int row = mousey / tilesize;
		int column = mousex / tilesize;
		if (currentunit != NULL) currentunit->buildFactory(units, tiles, factories, 2);
		currentunit = NULL;
		break;
	}
	case(SDLK_m):
	{
		int mousex;
		int mousey;
		SDL_GetMouseState(&mousex, &mousey);
		int row = mousey / tilesize;
		int column = mousex / tilesize;
		if (currentunit != NULL) currentunit->buildFactory(units, tiles, factories, 3);
		currentunit = NULL;
		break;
	}
	}

        return true;
}

void handle_mouseup(Uint8 button, std::vector<player*>& players,
                    unit* currentunit,
                    std::list<unit*>& units,
                    std::vector<std::vector<tile*>>& tiles,
                    SDL_Surface* winSurface, SDL_Window* window)
{
    if (button == SDL_BUTTON_LEFT)
    {
        // On left click, tell player to move to clicked tile
        int mousex;
        int mousey;
        SDL_GetMouseState(&mousex, &mousey);
        int row = mousey / tilesize;
        int column = mousex / tilesize;

        // Ignore clicks outside the board
        if (row >= tiles.size()) return;
        if (column >= tiles[0].size()) return;
        if (row < 0) return;
        if (column < 0) return;

        bool cont = true;
        for (auto unit : units)
        {
            if (unit->tileAt_->y_ == row && unit->tileAt_->x_ == column && unit->team_->human_)
            {
                currentunit = unit;
                cont = false;
            }
        }

        if (cont) // Only if currentunit is valid, then attempt to set a new path for currentunit
        {
            int targr = row;
            int targc = column;

            if (tiles[row][column]->state_ != 1 && tiles[row][column]->state_ != 3 && tiles[row][column]->magicflag == 62)
            {
                for (int r = 0; r < tiles.size(); r++)
                {
                    for (int c = 0; c < tiles[0].size(); c++)
                    {
                        tiles[r][c]->onpath = false;
                    }
                }
                // std::cout << "setting new goal to r=" << row << " and c=" << column << std::endl;
                // tiles[row][column]->state_ = 3;
                currentunit->navigate(tiles, units, tiles[row][column]);
            }
            else if (tiles[row][column]->magicflag != 62)
            {
                std::cout << "attempting to read data from a nonexistent tile" << std::endl;
            }
        }
    }
    else if (button == SDL_BUTTON_RIGHT)
    {
        // On right click, create new player at tile
        int mousex;
        int mousey;
        SDL_GetMouseState(&mousex, &mousey);
        int row = mousey / tilesize;
        int column = mousex / tilesize;
        if (players.size() == 0)
        {
            // std::cout << "Creating human player" << std::endl;
            players.push_back(new player(players.size(), *winSurface, true));
            units.push_back(new unit(players.back(), tiles, mainunit, row, column));
            currentunit = units.back();
        }
        else if (players.size() < playerlimit)
        {
            // std::cout << "Creating AI player" << std::endl;
            players.push_back(new player(players.size(), *winSurface, false));
            units.push_back(new unit(players.back(), tiles, mainunit, row, column));
        }
        else
        {
            std::cout << "Exceeded player limit, which is " << playerlimit << std::endl;
            std::system("pause");
            exit(1);
        }
        //players.back()->units_.push_back(units.back());
    }
    else if (button == SDL_BUTTON_MIDDLE)
    {
        // no-op
    }
}

// return code is a game status:
//  0: no winner yet, continue playing
// -1: stalemate, no player can win
// >0: team_id of winner (1-based)

int updateState(std::vector<player*>& players, unit* currentunit, std::list<unit*>& units, std::vector<std::vector<tile*>>& tiles, std::list<tile*>& factories, SDL_Surface* winSurface, SDL_Window* window)
{
	static Uint64 resourceTimer = SDL_GetTicks() % resourceMineInterval;
	static Uint64 unitSpawnTimer = SDL_GetTicks() % unitSpawnInterval;
	static Uint64 unitMoveTimer = SDL_GetTicks() % unitMoveInterval;
	static Uint64 aiActTimer = SDL_GetTicks() % aiActInterval;
        
        bool miningTimerDone = false;
        if (resourceTimer > SDL_GetTicks() % resourceMineInterval)
        {
                miningTimerDone = true;
        }
        resourceTimer = SDL_GetTicks() % resourceMineInterval;

        bool unitSpawnTimerDone = false;
        if (unitSpawnTimer > SDL_GetTicks() % unitSpawnInterval)
        {
                unitSpawnTimerDone = true;
        }
        unitSpawnTimer = SDL_GetTicks() % unitSpawnInterval;
                
        bool unitMoveTimerDone = false;
        if (unitMoveTimer > SDL_GetTicks() % unitMoveInterval)
        {
                unitMoveTimerDone = true;
        }
        unitMoveTimer = SDL_GetTicks() % unitMoveInterval;

        bool aiActTimerDone = false;
        if (aiActTimer > SDL_GetTicks() % aiActInterval)
        {
                aiActTimerDone = true;
        }
        aiActTimer = SDL_GetTicks() % aiActInterval;

        // Spawn units
        if (unitSpawnTimerDone)
        {
                for (auto factory : factories)
                {
                        factory->spawnUnit(tiles, units);
                }
        }

        // Cycle through every player, tell non-humans to perform AI actions
        if (aiActTimerDone)
        {
                for (auto playerPtr : players)
                {
                        if (!playerPtr->human_)
                        {
                                playerPtr->act(units, factories, tiles);
                        }
                }
        }

        // Cycle through every unit, compute combat, mining, and moving
        std::list<tile*> deadFactories;
        for (auto unitPtr : units)
        {
                //printf("player %d updating unit of type %d\n",unitPtr->team_->team_id_,unitPtr->type_);
                if (unitMoveTimerDone) unitPtr->unitMoveFlag = true;
                if (unitPtr->type_ == fighter)
                {
                        // Here we should not iterate through all units, but just check tiles
                        int r = unitPtr->tileAt_->y_;
                        int c = unitPtr->tileAt_->x_;

                        tile* lft_tile = tiles[r][c-1];
                        tile* rgt_tile = tiles[r][c+1];
                        tile* top_tile = tiles[r-1][c];
                        tile* bot_tile = tiles[r+1][c];

                        unit* lft = lft_tile->unitAt_;
                        unit* rgt = rgt_tile->unitAt_;
                        unit* top = top_tile->unitAt_;
                        unit* bot = bot_tile->unitAt_;

                        // if (lft) printf("  has lft nbr\n");
                        // if (rgt) printf("  has rgt nbr\n");
                        // if (top) printf("  has top nbr\n");
                        // if (bot) printf("  has bot nbr\n");

                        player* myteam = unitPtr->team_;
                        
                        std::vector<unit*> targets;
                        if (lft && lft->team_ != myteam) targets.push_back(lft);
                        if (rgt && rgt->team_ != myteam) targets.push_back(rgt);
                        if (top && top->team_ != myteam) targets.push_back(top);
                        if (bot && bot->team_ != myteam) targets.push_back(bot);
                        if (targets.size()) {
                                //printf("%lu targets\n",targets.size());
                                // char a = getchar();
                                // exit(1);
                        }

                        // choose a random target to attack
                        if (targets.size()) {
                                std::uniform_int_distribution<> targdist(0,targets.size()-1);
                                int itarget = targdist(gen);
                                //printf("choosing target %d\n",itarget);
                                unit* target = targets[itarget];

                                std::uniform_real_distribution<> hitdist(0, 1.0);
                                double ahit = hitdist(gen);

                                // 70% chance of hitting. If there's
                                // no randomness for hits, then two
                                // units fighting will always kill
                                // both, or kill one just by who takes
                                // the first turn.
                                
                                if (ahit < 0.5) {
                                        target->health_ -= 1;
                                }
                        }
                            
                        for(std::list<tile*>::iterator it = factories.begin(); it != factories.end(); it++)
                        {
                                tile* targetPtr = *it;
                                int targetX = targetPtr->x_;
                                int targetY = targetPtr->y_;
                                int selfX = unitPtr->tileAt_->x_;
                                int selfY = unitPtr->tileAt_->y_;

                                bool above = (targetX == selfX && targetY == selfY - 1);
                                bool left = (targetX == selfX - 1 && targetY == selfY);
                                bool right = (targetX == selfX + 1 && targetY == selfY);
                                bool below = (targetX == selfX && targetY == selfY + 1);
                                if ((above || left || right || below) && (targetPtr->claimedBy_ != unitPtr->team_))
                                {
                                        bool alreadyDeadCheck = deadFactories.end() == std::find(deadFactories.begin(), deadFactories.end(), targetPtr);
                                        if (alreadyDeadCheck)
                                        {
                                                targetPtr->claimedBy_ = NULL;
                                                targetPtr->factoryType = mainunit;
                                                targetPtr->state_ = 0;
                                                it = factories.erase(std::find(factories.begin(), factories.end(), targetPtr));
                                                if (it == factories.end()) break;
                                        }
                                }
                        }
                        
                } // fighter units

                if (miningTimerDone) unitPtr->resourceMineFlag = true;
                unitPtr->advance(tiles);
        }

        // kill units with 0 health
        std::list<unit*> deadUnits;
        for (auto unitPtr : units) {
                if (unitPtr->health_ <= 0) deadUnits.push_back(unitPtr);
        }

        // Kill units that died during this frame, avoids modifying actively iterated lists
        for (auto deadPtr : deadUnits)
        {
                if (deadPtr == currentunit) currentunit = NULL;
                player* deadPtrTeam = deadPtr->team_;
                std::list<unit*>::iterator deadUnit = std::find(deadPtrTeam->units_.begin(), deadPtrTeam->units_.end(), deadPtr);
                // check if dead unit is in its team's unit list
                if (deadUnit != deadPtrTeam->units_.end())
                {
                        //std::cout << "Dead unit found in units list. Pointer is " << deadPtr << std::endl;
                        deadPtrTeam->units_.erase(deadUnit);
                }
                else std::cout << "Dead unit not found in team's units list. Pointer is " << deadPtr << std::endl;


                units.erase(std::find(units.begin(), units.end(), deadPtr));
                deadPtr->tileAt_->unitAt_ = nullptr; // "corpse" removed from tile
                delete deadPtr;
        }


        // Win conditions: if player has no factories or units, it is
        // a dead player, and if only one player left and has units
        // and factories, that player wins
        std::list<player*> deadPlayers;
        for (auto playerPtr : players)
        {
                bool hasNoFighters = true;
                for (auto unit : playerPtr->units_) {
                        if (unit->type_ == mainunit) hasNoFighters = false;
                        if (unit->type_ == fighter) hasNoFighters = false;
                }
                
                bool hasNoFactories = true;
                for (auto factoryPtr : factories)
                {
                        if (factoryPtr->claimedBy_ == playerPtr) {
                                hasNoFactories = false;
                                break;
                        }
                }
                bool hasNoUnits = !playerPtr->units_.size();
                if (hasNoFighters || (hasNoUnits && hasNoFactories))
                {
                        deadPlayers.push_back(playerPtr);
                }
        }
        for (auto playerPtr : deadPlayers)
        {
                // deleting dead player
                delete playerPtr;
                players.erase(std::find(players.begin(), players.end(), playerPtr));
        }

        if (players.size() == 0) {
                return -1;
        }
        

        if (players.size() == 1)
        {
                return players[0]->team_id_;
        }

        // Check for stalemate. A stalemate occurs when no player has
        // a fighters, fighter factories, or main units left.
        bool playerHasFighters = false;
        for (auto playerPtr : players)
        {
                for (auto unit : playerPtr->units_) {
                        if (unit->type_ == mainunit) playerHasFighters = true;
                        if (unit->type_ == fighter) playerHasFighters = true;
                        if (playerHasFighters) break;
                }
                // TODO: check for fighter factories
        }
        if (!playerHasFighters) return -1; // stalemate
        
        return 0; // no winner, continue playing
}

int initSDL(SDL_Surface*& winSurface, SDL_Window*& window, int winx, int winy)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		std::cout << "Error initializing SDL. " << SDL_GetError() << std::endl;
		std::system("pause");
		return 1;
	}

	window = SDL_CreateWindow("RTSGame", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, winx, winy, SDL_WINDOW_SHOWN);
	if (!window) 
	{
		std::cout << "Error creating window. " << SDL_GetError() << std::endl;
		std::system("pause");
		return 1;
	}

	winSurface = SDL_GetWindowSurface(window);
	if (!winSurface)
	{
		std::cout << "Error getting surface. " << SDL_GetError() << std::endl;
		std::system("pause");
		return 1;
	}

        return 0;
}

// run match and return game result.
// >0: team_id of winner
//  0: stalemate; no winner

int runMatch(std::vector<player*>& players, SDL_Surface* winSurface, SDL_Window* window)
{
	// Map init
	std::vector<std::vector<tile*>> tiles;
	initMap(tiles, false, false);

	std::list<unit*> units;

        // Each player starts with 1 unit

        int nr = tiles.size();
        int nc = tiles[0].size();
        
        maxr = nr-1;
        maxc = nc-1;
        
        double ra = rand()/double(RAND_MAX);
        double rb = rand()/double(RAND_MAX);

        int maxr0 = maxr-2;
        int minr0 = 2;

        int maxc0 = maxc/2-2;
        int minc0 = 2;
        
        int row0 = minr0 +ra*(maxr0-minr0);
        int col0 = minc0 +rb*(maxc0-minc0);
        
        int row1 = maxr-row0;
        int col1 = maxc-col0;

        units.push_back(new unit(players[0], tiles, mainunit, row0, col0));
        units.push_back(new unit(players[1], tiles, mainunit, row1, col1));

	// Create unit, initialize, create path variable
	
	unit* currentunit = NULL;

	// Create list of factories, so that not every tile has to be searched for spawning loop
	std::list<tile*> factories;

	// Event loop
	bool gameRunning = true;
	SDL_Event event;

        int game_status;
        
	// Main game loop
	while (gameRunning)
	{
		Uint64 start = SDL_GetPerformanceCounter();

                // Process events
		while (SDL_PollEvent(&event)) {

			switch (event.type)
			{
			case(SDL_QUIT):
                                goto gameover;
			case(SDL_KEYDOWN):
                            gameRunning = handle_keydown(
                                event.key.keysym.sym, players, currentunit,
                                units, tiles, factories, winSurface, window);
                            if (!gameRunning) goto gameover;
			case(SDL_MOUSEBUTTONUP):
                            // handle_mouseup(event.button.button, players, currentunit,
                            //                units, tiles, winSurface, window);
                            break;
			}
		}

                game_status = updateState(players, currentunit, units, tiles,
                                          factories, winSurface, window);

		if (game_status == 0) {
                        drawMap(winSurface, window, tiles, units, players);
                }
                else {
                        break;
                }
                
		// FPS counter
		// Uint64 end = SDL_GetPerformanceCounter();
		// double elapsed = (end - start) / double(SDL_GetPerformanceFrequency());
		// int FPS = 1 / elapsed;
                // std::cout << "FPS is " << FPS << std::endl;
	}

gameover:
        // stalemate
        if (game_status == -1) return 0;

        // winner id
        return game_status;
}

int main(int argc, char** args)
{
        
        
	SDL_Surface* winSurface = NULL;
	SDL_Window* window = NULL;

        // read map so we can get dimensions for window
        int winw,winh;
        {
                std::vector<std::vector<tile*>> tiles;
                initMap(tiles, false, false);
                int maxr = tiles.size();
                int maxc = tiles[0].size();

                const int boardlox = 100+2*tilesize;
                const int boardloy = 0;
                
                winw = boardlox+maxc*tilesize;
                winh = boardloy+maxr*tilesize;
        }
        
        int err = initSDL(winSurface, window, winw, winh);
        if (err) exit(err);

        // AI vs AI
	std::vector<player*> players;
        players.push_back(new player(0, *winSurface, false));
        players.push_back(new player(1, *winSurface, false));

        // Run tournament
        int N = 10;
        int player1wins = 0;
        int player2wins = 0;
        for (int n = 0; n < N; n++) {

                printf("Running match %d/%d\n",n+1,N);
                {
                        std::vector<player*> players;
                        players.push_back(new player(0, *winSurface, false));
                        players.push_back(new player(1, *winSurface, false));
                        
                        int match_result = runMatch(players, winSurface, window);

                        if (!match_result) {
                                printf("match %d/%d is a stalemate\n",n+1,N);
                        }
                        else {
                                printf("player %d wins match %d/%d\n",match_result,n+1,N);
                                delete players[match_result-1];
                                if (match_result == 1) player1wins++;
                                if (match_result == 2) player2wins++;
                        }
                }
        }
        int ties = N-player1wins-player2wins;
        printf("%f|%f|%f\n",player1wins/double(N)*100,player2wins/double(N)*100,ties/double(N)*100);

	// Cleanup
	SDL_FreeSurface(winSurface);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
