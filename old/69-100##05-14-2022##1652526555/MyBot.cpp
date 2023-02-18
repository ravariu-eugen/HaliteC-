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
#define MAX_TURN 300
#define MAX_STRENGTH 255

#define POTENTIAL_WEIGHT 2.0
#define POTENTIAL_DECAY 0.4

#define SIGHT_RADIUS 3

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
        uint8_t direction = NORTH;
        if(locStren(loc) < 5*locProd(loc))
            return STILL;
        std::vector<int> possibleMoves;
        if(loc.x%3==0 || loc.y%3==0){
            if(loc.x%3==0){
                possibleMoves.push_back(NORTH);
                possibleMoves.push_back(SOUTH);
            }
            if(loc.y%3==0){
                possibleMoves.push_back(EAST);
                possibleMoves.push_back(WEST);
            }
        }
        else{
            if(loc.x%3==1)            
                possibleMoves.push_back(WEST);
            else if(loc.x%3 ==2)
                possibleMoves.push_back(EAST);
            if(loc.y%3==1)            
                possibleMoves.push_back(NORTH);
            else if(loc.y%3 ==2)
                possibleMoves.push_back(SOUTH);
        }
        
        

        double bestPotential = 0;
        for (auto i:CARDINALS) {

            hlt::Location neighborLocation = gameMap.getLocation(loc, i);
            if(potential[neighborLocation.x][neighborLocation.y] > bestPotential){
                bestPotential = potential[neighborLocation.x][neighborLocation.y];
                direction = i;
            }


        }

        return direction;
    }
    bool isContested(hlt::Location loc){
        bool isContested = false;
        bool enemy = false;
        bool neutral = false;
        for(auto i : DIRECTIONS){
            enemy = enemy || locOwner(loc, i) == myID ;
            neutral = neutral || locOwner(loc, i) == myID ;
        }
        return enemy && neutral;
    }
    int enemyConcentration(hlt::Location loc){
        int sum = 0;
        for(auto i : DIRECTIONS){
            sum += locStren(loc, i) * locProd(loc, i);
        }
        return sum;
    }
    int tileValue(hlt::Location loc){
        hlt::Site site = gameMap.getSite(loc);
        int p = 1000;
        if(conflictZone(loc) > 0)
            p*=conflictZone(loc);
        return p*site.production*site.production/(1+ site.strength);
    
    }
    bool onBorder(hlt::Location loc){
        for (auto i : CARDINALS){
            if(locOwner(loc, i) != myID)
            return true;
        }
        return false;
    }

    bool isEnemy(hlt::Location loc){
        return locOwner(loc) != myID && locOwner(loc) != 0;
    }
    hlt::Location findEnemy(hlt::Location loc){

        for(auto i = 1; i <= SIGHT_RADIUS; i++){
            for(auto j = 0; j < i; j++){
                
                hlt::Location nloc = {loc.x+j,loc.y-i};//1
                if(inBounds(nloc)){
                    if(isEnemy(nloc))
                        return nloc;
                }
                nloc = {loc.x+i,loc.y+j};//2
                if(inBounds(nloc)){
                    if(isEnemy(nloc))
                        return nloc;
                }
                nloc = {loc.x-j,loc.y+i};//3
                if(inBounds(nloc)){
                    if(isEnemy(nloc))
                        return nloc;
                }
                nloc = {loc.x-i,loc.y-j};//4
                if(inBounds(nloc)){
                    if(isEnemy(nloc))
                        return nloc;
                }
                
            }
        }
        return {1<<15,1<<15};
    }

    int moveValue(hlt::Move move){
        hlt::Site currentSite = gameMap.getSite(move.loc);
        if(move.dir == STILL){
            //if(currentSite.production + currentSite.strength <= MAX_STRENGTH)
            return currentSite.production + locPotential(move.loc, move.dir);
            //else return 0;
        }
        hlt::Site neighborSite = gameMap.getSite(move.loc, move.dir);
        //if(isContested(gameMap.getLocation(move.loc, move.dir)))
        //return enemyConcentration(gameMap.getLocation(move.loc, move.dir)) + locPotential(move.loc, move.dir);
        if(neighborSite.owner == 0){
            if(conflictZone(move.loc, move.dir) >0)
                return 1<<20 * conflictZone(move.loc, move.dir);
            if(neighborSite.strength >= currentSite.strength){
                return -currentSite.strength;// + locPotential(move.loc, move.dir);
            }
            else
            {
                ofs << remainingTurns() << std::endl;
                return tileValue(gameMap.getLocation(move.loc, move.dir)) + locPotential(move.loc, move.dir);
            }
        }
        else if(neighborSite.owner == currentSite.owner){
            return 1000 * std::min(0, MAX_STRENGTH - (currentSite.strength + neighborSite.strength)) + locPotential(move.loc, move.dir);

        }
        else {
            return 1<<25;
            /*int dealtDamage = 0;
            int receivedDamage = 0;
            hlt::Location neighborLocation = gameMap.getLocation(move.loc, move.dir);
            for(auto i : DIRECTIONS){
                hlt::Site contactSite = gameMap.getSite(neighborLocation, i);

                if(contactSite.owner != myID && contactSite.owner != 0){
                    dealtDamage += std::min(contactSite.strength, neighborSite.strength);
                    receivedDamage = std::max(receivedDamage + contactSite.strength, (int)neighborSite.strength);
                }   
            }
            if(receivedDamage == neighborSite.strength)
                return dealtDamage - receivedDamage;
            else 
                return dealtDamage - receivedDamage + locPotential(move.loc, move.dir);
                */
            } 
    }

    int conflictZone(hlt::Location loc, unsigned char direction = STILL){
        int s = 0;
        if(locStren(loc) > 0) return -5;
        for(auto i : CARDINALS){
            if(locOwner(loc, i) != myID && locOwner(loc, i) != 0)
                s++;
        }
        return s;
    }



    int remainingTurns(){
        return maxTurns - turnCount;
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
    bool inBounds(hlt::Location loc, unsigned char direction = STILL){
        hlt::Location neighborLocation = gameMap.getLocation(loc, direction);
        return neighborLocation.x >= 0 && neighborLocation.x < gameMap.width 
            && neighborLocation.y >= 0 && neighborLocation.y < gameMap.height;
    }
    unsigned char getDirectionTo(hlt::Location loci, hlt::Location locf){

        if(loci == locf)
            return STILL;

        if(loci.x - loci.y > locf.x - locf.y)
            if(loci.x + loci.y > locf.x + locf.y) return NORTH;
            else return EAST;
        else 
            if(loci.x + loci.y > locf.x + locf.y) return WEST;
            else return SOUTH;

    }
    void initPotential(){

        this->potential = std::vector< std::vector<double> >
                            (gameMap.width, std::vector<double> (gameMap.height, 0));
        int visited[gameMap.width+5][gameMap.height+5] = {};
        std::priority_queue<locVal> locations;
        for(unsigned short y = 0; y < gameMap.height; y++) {
            for(unsigned short x = 0; x < gameMap.width; x++)
            if(locOwner({x,y})!=myID) 
            {
                
                //potential[x][y] = tileValue({x,y});
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
            for(auto i:CARDINALS){
                hlt::Location neighborLocation = gameMap.getLocation(currentBestLoc, i);
                if(visited[neighborLocation.x][neighborLocation.y] == 2)
                    if(tileValue(neighborLocation) > maxNeighbourVal){
                        maxNeighbourVal = tileValue(neighborLocation);
                        maxNeighbourDir = i;
                    }
            }
            if(locOwner(currentBestLoc) == myID){
                potential[currentBestLoc.x][currentBestLoc.y] = POTENTIAL_DECAY * locPotential(currentBestLoc, maxNeighbourDir);
            }
            else {
                potential[currentBestLoc.x][currentBestLoc.y] = (1.0*tileValue(currentBestLoc) 
                                                                + POTENTIAL_WEIGHT * locPotential(currentBestLoc, maxNeighbourDir)
                                                                )
                                                                / (1.0+ POTENTIAL_WEIGHT);
            }
            for(auto i:CARDINALS){
                hlt::Location neighborLocation = gameMap.getLocation(currentBestLoc, i);
                
                if(visited[neighborLocation.x][neighborLocation.y] != 2){
                    visited[neighborLocation.x][neighborLocation.y] = 1;
                    locations.push({neighborLocation,potential[currentBestLoc.x][currentBestLoc.y]});
                }
            }
            
        }
        

    }   


    void resetStrenMap(){
        for(int i = 0; i < gameMap.width; i++){
            for(int j = 0; j < gameMap.height; j++){
                if(locOwner({i,j}) == myID)
                    strenMap[i][j] = locStren({i,j});
                else 
                    strenMap[i][j] = 0;
            }   
        }
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

            int bestVal = moveValue({loc,STILL});
            ofs << 0 << " " << bestVal << "\n";
            for(auto i : CARDINALS) {
        
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
            bestDir = findBorder(loc);
        }
        if(site.strength < 5*site.production && locOwner(loc, bestDir) == myID)
            return {loc,STILL};
        if(locStren(loc, bestDir) >= site.strength && locOwner(loc, bestDir) != myID)
                return{loc,STILL};
       // if(locStren(loc, bestDir) + site.strength > MAX_STRENGTH && locOwner(loc, bestDir) == myID)
       //         return{loc,STILL};
        hlt::Location neighborLocation = gameMap.getLocation(loc, bestDir);
        if(locStren(loc) + strenMap[neighborLocation.x][neighborLocation.y] >= MAX_STRENGTH + 20)
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
                    moves.insert(finder.assignMove({b,a}));
                }
            }
        }
        
    

        sendFrame(moves);
    }

    return 0;
}
