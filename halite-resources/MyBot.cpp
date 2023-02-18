#include <stdlib.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <set>
#include <fstream>
#include <cmath>

#include "hlt.hpp"
#include "networking.hpp"
#include <queue>
#include <iomanip>
#define MAX_STRENGTH 255
#define MAX_OVERFLOW 10

#define POTENTIAL_WEIGHT 1.5
#define POTENTIAL_DECAY 0.70

#define GO_FRACTION 5

std::ofstream ofs("log.txt");

class MoveFinder{
private:
    hlt::GameMap gameMap;
    int myID;
    int turnCount;
    int maxTurns;
    std::vector<std::vector<double> > potential;
    std::vector<std::vector<int> > strenMap; 
    uint8_t findBorder(hlt::Location loc) {
        // directia catre granita
        uint8_t direction = rand()%4+1;
        // alege directia cu potential cel mai bun
        // intrucat potentialul scade exponential de la granita
        // se va ajunge mereu la granita
        double bestPotential = 0;
        for (auto i:CARDINALS) {

            hlt::Location neighborLocation = gameMap.getLocation(loc, i);
            if(locStren(loc) + strenMap[neighborLocation.x][neighborLocation.y] >= MAX_STRENGTH + MAX_OVERFLOW)
                continue;
            if(potential[neighborLocation.x][neighborLocation.y] > bestPotential){
                bestPotential = potential[neighborLocation.x][neighborLocation.y];
                direction = i;
            }
        }

        return direction;
    }
    int tileValue(hlt::Location loc){
        // valoarea unei locatii
        hlt::Site site = gameMap.getSite(loc);
        int p = 1000;
        if(conflictZone(loc) > 0)
            p*=conflictZone(loc);
        if(isEnemy(loc))
            p*=turnCount/maxTurns*(turnCount>maxTurns/3);
        return p*site.production*site.production/(1+ site.strength) ;
    
    }
    bool onBorder(hlt::Location loc){
        // daca locatia este la granita
        for (auto i : CARDINALS){
            if(locOwner(loc, i) != myID)
            return true;
        }
        return false;
    }

    bool isEnemy(hlt::Location loc){
        // daca detinatorul locatiei este inamic
        return locOwner(loc) != myID && locOwner(loc) != 0;
    }
   
    int conflictZone(hlt::Location loc, unsigned char direction = STILL){
        // o zona de conflict este o locatie neutra 
        int s = 0;
        if(locStren(loc, direction) > 0 || locOwner(loc, direction) != 0) 
            return -5;
        for(auto i : CARDINALS){
            if(locOwner(loc, i) != myID && locOwner(loc, i) != 0)
                s++;
        }
        return s;
    }

    int moveValue(hlt::Move move){
        // valoarea unei miscari
        // pentru locatiile de la granita
        hlt::Site currentSite = gameMap.getSite(move.loc);
        if(move.dir == STILL){
            return currentSite.production + locPotential(move.loc, move.dir);
        }
        hlt::Site neighborSite = gameMap.getSite(move.loc, move.dir);
        
        if(neighborSite.owner == 0){
            if(conflictZone(move.loc, move.dir) >0)
                // caz pentru zona de conflict
                return 1<<20 * conflictZone(move.loc, move.dir);
            // caz pentru zona neutra fara conflict
            if(neighborSite.strength >= currentSite.strength){
                return -currentSite.strength + locPotential(move.loc, move.dir);
            }
            else
            {
                return tileValue(gameMap.getLocation(move.loc, move.dir)) + locPotential(move.loc, move.dir);
            }
        }
        else if(neighborSite.owner == currentSite.owner){
            // caz pentru miscare interna
            // chiar daca e la granita, e posibil ca potentialul maxim sa fie altundeva
            return 1000 * std::min(0, MAX_STRENGTH - (currentSite.strength + neighborSite.strength)) + locPotential(move.loc, move.dir);

        }
        else {
            // caz pentru locatie inamica
            return 1<<25;
            
            } 
    }
    
    struct locVal{
        hlt::Location loc;
        double val;
        bool operator<(locVal const loc)const{
            return val < loc.val;
        }
    };
    
    int locOwner(hlt::Location loc, unsigned char direction = STILL){
        return gameMap.getSite(loc, direction).owner;
    }
    int locProd(hlt::Location loc, unsigned char direction = STILL){
        return gameMap.getSite(loc, direction).production;
    }
    int locStren(hlt::Location loc, unsigned char direction = STILL){
        return gameMap.getSite(loc, direction).strength;
    }
    double locPotential(hlt::Location loc, unsigned char direction = STILL){
        hlt::Location neighborLocation = gameMap.getLocation(loc, direction);
        return potential[neighborLocation.x][neighborLocation.y];
    }
    
    void initPotential(){
        // calculeaza potentialul hartii
        this->potential = std::vector< std::vector<double> >
                            (gameMap.width, std::vector<double> (gameMap.height, 0));
        int visited[gameMap.width+5][gameMap.height+5] = {};
        std::priority_queue<locVal> locations;
        for(unsigned short y = 0; y < gameMap.height; y++) {
            for(unsigned short x = 0; x < gameMap.width; x++)
            if(locOwner({x,y})!=myID) 
            {
                // gaseste varfurile
                bool isPeak = true;
                for(auto i : CARDINALS){
                    if(tileValue({x,y}) < tileValue(gameMap.getLocation({x,y},i))){
                        isPeak = false;
                        break;
                    }
                }
                if(isPeak){
                    visited[x][y] = 2;
                    potential[x][y] = tileValue({x,y});
                    // seteaza potentialul si pune vecinii in coada de prioritati
                    for(auto i : CARDINALS){
                        hlt::Location neighborLocation = gameMap.getLocation({x,y}, i);
                        if(visited[neighborLocation.x][neighborLocation.y] != 2)
                        locations.push({neighborLocation, potential[x][y]});  
                    }
                }
            }
        }
        while(!locations.empty()){
            hlt::Location currentBestLoc = locations.top().loc;
            locations.pop();
            if(visited[currentBestLoc.x][currentBestLoc.y] == 2)
                continue;
            visited[currentBestLoc.x][currentBestLoc.y] = 2;
            int maxNeighbourVal = 0;
            int maxNeighbourDir = NORTH;
            // gaseste directia cu potential maxim
            for(auto i:CARDINALS){
                hlt::Location neighborLocation = gameMap.getLocation(currentBestLoc, i);
                if(visited[neighborLocation.x][neighborLocation.y] == 2)
                    if(locPotential(neighborLocation) > maxNeighbourVal){
                        maxNeighbourVal = locPotential(neighborLocation);
                        maxNeighbourDir = i;
                    }
            }
            // calculeaza potentialul
            if(locOwner(currentBestLoc) == myID){
                potential[currentBestLoc.x][currentBestLoc.y] = POTENTIAL_DECAY * (1.1-proportion()*proportion()) * locPotential(currentBestLoc, maxNeighbourDir);
            }
            else {
                potential[currentBestLoc.x][currentBestLoc.y] = (1.0*tileValue(currentBestLoc) 
                                                                + POTENTIAL_WEIGHT * locPotential(currentBestLoc, maxNeighbourDir)
                                                                )
                                                                / (1.0+ POTENTIAL_WEIGHT);
            }
            for(auto i:CARDINALS){
                hlt::Location neighborLocation = gameMap.getLocation(currentBestLoc, i);
                // pune vecinii in coada de prioritati
                if(visited[neighborLocation.x][neighborLocation.y] != 2){
                    visited[neighborLocation.x][neighborLocation.y] = 1;
                    locations.push({neighborLocation,potential[currentBestLoc.x][currentBestLoc.y]});
                }
            }
            
        }
        

    }   

    void resetStrenMap(){
        for(unsigned short i = 0; i < gameMap.width; i++){
            for(unsigned short j = 0; j < gameMap.height; j++){
                if(locOwner({i,j}) == myID)
                    strenMap[i][j] = locStren({i,j});
                else 
                    strenMap[i][j] = 0;
            }   
        }
    }

    double proportion(){
        return 1.0 * turnCount/maxTurns;
    }

    void moveStren(hlt::Location loc, unsigned char direction){
        strenMap[loc.x][loc.y] = 0;
        hlt::Location neighborLocation = gameMap.getLocation(loc, direction);
        strenMap[neighborLocation.x][neighborLocation.y] = std::max(MAX_STRENGTH, locStren(neighborLocation) + locStren(loc));
    }

public:
    MoveFinder(){
        turnCount = 1;
    }
    MoveFinder(hlt::GameMap& gameMap, int myID, int maxTurns){
        MoveFinder();
        turnCount = 1;
        this->myID = myID;
        this->gameMap = gameMap;
        this->maxTurns = maxTurns;
        initPotential();
        strenMap = std::vector< std::vector<int>>(gameMap.width, std::vector<int>(gameMap.height, 0));
        resetStrenMap();
    }
    ~MoveFinder(){

    }
    void updateFinder(hlt::GameMap& gameMap){
        this->gameMap = gameMap;
        this->turnCount++;
        initPotential();
        resetStrenMap();
    }
    // for each tile, what is the best move?
    hlt::Move assignMove(hlt::Location loc){
        
        ofs << "loc: "<< loc.x << " " << loc.y << "\n";
        hlt::Move chosenMove;
        hlt::Site site = gameMap.getSite(loc,STILL);
        uint8_t bestDir = STILL;

        if(onBorder(loc)){
            // daca este la margine
            // alege miscarea de valoare maxima
            int bestVal = moveValue({loc,STILL});
            ofs << 0 << " " << bestVal << "\n";
            for(unsigned char i : CARDINALS) {
        
                hlt::Move move = {loc, i};
                int val = moveValue(move);
                if(val > bestVal)
                {
                    bestVal = val;
                    bestDir = i;
                }
    
            }
        }
        else{
            // altfel, merge catre margine
            bestDir = findBorder(loc);
        }
        if(site.strength < (GO_FRACTION * (0.5 + 2*proportion())*site.production) && locOwner(loc, bestDir) == myID)
            return {loc,STILL}; 
        
        // evita miscarile ce nu obtin nimic
        if(locStren(loc, bestDir) >= site.strength && locOwner(loc, bestDir) != myID)
                return{loc,STILL};


        // prevenire overflow
        //if(locStren(loc, bestDir) + site.strength > MAX_STRENGTH + MAX_OVERFLOW && locOwner(loc, bestDir) == myID)
        //        return{loc,STILL};
        hlt::Location neighborLocation = gameMap.getLocation(loc, bestDir);
        if(locStren(loc) + strenMap[neighborLocation.x][neighborLocation.y] >= MAX_STRENGTH + MAX_OVERFLOW)
            bestDir = STILL;
        else
            moveStren(loc, bestDir); 

        chosenMove = {loc, bestDir};
        return chosenMove;
    }
};





int main() {
    srand(time(NULL));

    std::cout.sync_with_stdio(0);

    unsigned char myID;
    hlt::GameMap presentMap;
    getInit(myID, presentMap);
    sendInit("MyC++Bot");

    std::set<hlt::Move> moves;
    int turnCount = 0;
    int maxTurns = 10 * ((int) std::sqrt(presentMap.width*presentMap.height));
    ofs << maxTurns << std::endl;
    MoveFinder finder(presentMap, myID, maxTurns);
    while(true) {
        moves.clear();
        turnCount++;
        getFrame(presentMap);
        finder.updateFinder(presentMap);
        for(unsigned short a = 0; a < presentMap.height; a++) {
            for(unsigned short b = 0; b < presentMap.width; b++) {
                if (presentMap.getSite({ b, a }).owner == myID) {
                    // alegere miscare
                    moves.insert(finder.assignMove({b,a}));
                }
            }
        }
        sendFrame(moves);
    }

    return 0;
}