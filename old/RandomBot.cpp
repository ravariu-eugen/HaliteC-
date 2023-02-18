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

#define MAX_TURN 300
#define MAX_STRENGTH 255

std::ofstream ofs("log.txt");


class MoveFinder{
private:
    hlt::GameMap gameMap;
    int myID;
    int turnCount;
    int maxTurns;
    std::vector<std::vector<int>> strength;
    std::vector<std::vector<int>> enemyConcentration;
    uint8_t findNearestEnemyDirection(hlt::Location loc) {
        uint8_t direction = NORTH;
        // don't get stuck in an infinite loop
        int maxDistance = std::min(gameMap.width, gameMap.height) / 2;
        for (auto i:CARDINALS) {
            int distance = 0;
            hlt::Location current = loc;
            hlt::Site site = gameMap.getSite(current, i);
            int id = site.owner;
            while (site.owner == id && distance < maxDistance) {
                distance++;
                current = gameMap.getLocation(current, i);
                site = gameMap.getSite(current);
            }

            if (distance < maxDistance) {
                direction = i;
                maxDistance = distance;
            }
        }

        return direction;
    }
    int tileValue(hlt::Location loc){
        hlt::Site site = gameMap.getSite(loc);
        return remainingTurns() * site.production - site.strength; 
    }
    bool onBorder(hlt::Location loc){
        for (auto i : CARDINALS){
            if(gameMap.getSite(loc, i).owner != gameMap.getSite(loc, STILL).owner)
            return true;
        }
        return false;
    }
    int moveValue(hlt::Move move){
        hlt::Site currentSite = gameMap.getSite(move.loc);
        if(move.dir == STILL)
            return currentSite.production;
        hlt::Site neighborSite = gameMap.getSite(move.loc, move.dir);
        
        if(neighborSite.owner == 0){
            if(neighborSite.strength >= currentSite.strength){
                return -currentSite.strength;
            }
            else
            {
                return remainingTurns() * neighborSite.production - neighborSite.strength;
            }
        }
        else if(neighborSite.owner == currentSite.owner){
            return std::min(0, MAX_STRENGTH - (currentSite.strength + neighborSite.strength));

        }
        else {
            int dealtDamage = 0;
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
                return dealtDamage - receivedDamage + remainingTurns() * neighborSite.production;
        } 
    }
    int remainingTurns(){
        return maxTurns - turnCount;
    }
    void getStrength(){
        for(int i = 0; i < gameMap.width; i++){
            for(int j = 0; j < gameMap.height; j++){
                strength[i][j] = gameMap.contents[i][j].strength;
            }
        }
    }
    int getCurrentStrength(hlt::Location loc){
        return strength[loc.x][loc.y];
    }

public:
    MoveFinder(){
        turnCount = 1;
    }
    MoveFinder(hlt::GameMap& gameMap, int myID, int turnCount, int maxTurns){
        MoveFinder();
        this->gameMap = gameMap;
        this->turnCount = turnCount;
        this->maxTurns = maxTurns;
        //getStrength();
    }
    ~MoveFinder(){

    }
    // for each tile, what is the best move?
    hlt::Move assignMove(hlt::Location loc){
        
        ofs << "loc: "<< loc.x << " " << loc.y << "\n";
        hlt::Move chosenMove;
        if(onBorder(loc)){
            chosenMove = {loc,STILL};
            int bestVal = moveValue(chosenMove);
            ofs << 0 << " " << bestVal << "\n";
            for(auto i : CARDINALS) {
        
                hlt::Move move = {loc, i};
                int val = moveValue(move);
                if(val > bestVal)
                {
                    bestVal = val;
                    chosenMove = move;
                }
                ofs << i << " " << val << "\n";
            }
            ofs << (int)chosenMove.dir << " " << bestVal << "\n";
            return chosenMove;
        }
        hlt::Site site = gameMap.getSite(loc,STILL);
        if(site.strength < 32)
            return {loc,STILL};

        uint8_t bestDir = findNearestEnemyDirection(loc);
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
    sendInit("Old++Bot");


    

    std::set<hlt::Move> moves;
    int turnCount = 0;
    int maxTurns = 10 * ((int) std::sqrt(presentMap.width*presentMap.height));
    while(true) {
        moves.clear();
        turnCount++;
        getFrame(presentMap);
        MoveFinder finder(presentMap, myID, turnCount, maxTurns);
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
