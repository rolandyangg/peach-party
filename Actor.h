#ifndef ACTOR_H_
#define ACTOR_H_

#include "GraphObject.h"

// Students:  Add code to this file, Actor.cpp, StudentWorld.h, and StudentWorld.cpp
#include <set>
#include <map>
class StudentWorld;

// TODO: ORGANIZE CODE, REDUCE REDUNDANCY

///////////////////////////////////////////////////////////////////////////
//  CONSTANTS
///////////////////////////////////////////////////////////////////////////
const int PEACH = 1;
const int YOSHI = 2;
const int WAITING_TO_ROLL = false;
const int PAUSED = false;
const int WALKING = true;
const int invalid_direction = -100;

//***************************************************************************//

class Actor: public GraphObject {
public:
    Actor(int imageID, int startX, int startY, StudentWorld* world, int depth = 0, int dir = right, double size = 1.0, bool active = true):
    GraphObject(imageID, startX, startY, dir, depth, size),
    m_active(active),
    m_world(world) {}
    virtual ~Actor() { std::cerr << "Actor Deconstructor Called" << std::endl; }
    
    // VIRTUAL FUNCTIONS
    virtual void doSomething() = 0;
    virtual void doImpactedBehavior() = 0;
    virtual bool isStatic() = 0;
    virtual bool isImpactable() = 0;
    virtual bool isDynamicallyAdded() = 0;
    
    // TRIVIAL FUNCTIONS
    bool isActive() { return m_active; }
    void setActive(bool active) { m_active = active; }
    StudentWorld* getWorld() { return m_world; }
    
private:
    bool m_active;
    StudentWorld* m_world;
};

//***************************************************************************//

class Character: public Actor {
public:
    Character(int imageID, int startX, int startY, StudentWorld* world):
    Actor(imageID, startX, startY, world),
    m_ticksToMove(0),
    m_state(false),
    m_walkDirection(0) {}
    virtual ~Character() {}
    
    // NON-TRIVIAL FUNCTIONS
    void swapPhysicalState(Character* otherCharacter);
    void updateSpriteDirection();
    void handleCornerTurn();
    int getRandomDirection();
    bool canMoveInDirection(int dir);
    bool isAtFork();
    
    // TRIVIAL FUNCTIONS
    virtual bool isStatic() { return false; }
    int getTicks() const { return m_ticksToMove; }
    bool getState() const { return m_state; }
    int getWalkDirection() const { return m_walkDirection; }
    void setTicks(int ticks) { m_ticksToMove = ticks; }
    void setState(bool state) { m_state = state; }
    void setWalkDirection(int walkDirection) { m_walkDirection = walkDirection; }
    bool isLegalDirectionChange(int dir) { return (canMoveInDirection(dir)) && (((getWalkDirection() + 180) % 360) != dir); }
private:
    int m_ticksToMove;
    bool m_state;
    int m_walkDirection;
};

//***************************************************************************//

class Player: public Character {
public:
    Player(int playerNumber, int startX, int startY, StudentWorld* world):
    Character(playerNumber == 1 ? IID_PEACH : IID_YOSHI, startX, startY, world),
    m_playerNumber(playerNumber),
    m_coins(0),
    m_stars(0),
    m_vortex(false),
    m_isNew(true) {}
    virtual ~Player() {}
    
    // NON-TRIVIAL FUNCTIONS
    virtual void doSomething();
    virtual void doImpactedBehavior() {};
    int getDirectionFromAction(int playerAction);
    
    // TRIVIAL FUNCTIONS
    virtual bool isImpactable() { return false; }
    virtual bool isDynamicallyAdded() { return false; }
    int getCurrentRoll() const { return ceil(getTicks() / 8); }
    int getCoins() const { return m_coins; }
    int getStars() const { return m_stars; }
    bool hasVortex() const { return m_vortex; }
    bool isNew() const { return m_isNew; }
    void setCoins(int coins) { m_coins = coins; }
    void setStars(int stars) { m_stars = stars; }
    void setVortex(bool vortex) { m_vortex = vortex; }
    void setNew(bool isNew) { m_isNew = isNew; }
private:
    int m_playerNumber; // 1 = PEACH, 2 = YOSHI
    int m_coins;
    int m_stars;
    bool m_vortex;
    bool m_isNew;
};

class Vortex: public Actor {
public:
    Vortex(int dir, int startX, int startY, StudentWorld* world):
    Actor(IID_VORTEX, startX, startY, world, 0, dir) {}
    virtual ~Vortex() {}
    
    virtual void doSomething();
    virtual void doImpactedBehavior() {}
    
    virtual bool isStatic() { return false; }
    virtual bool isImpactable() { return false; }
    virtual bool isDynamicallyAdded() { return true; }
private:
    
};

class Baddie: public Character {
public:
    Baddie(int maxSquaresToMove, int imageID, int startX, int startY, StudentWorld* world):
    Character(imageID, startX, startY, world),
    m_maxSquaresToMove(maxSquaresToMove),
    m_pauseCounter(180) { m_playersSharingSquare = new std::set<Player*>; }
    virtual ~Baddie();
    
    // NON-TRIVIAL FUNCTIONS
    virtual void doSomething();
    virtual void doImpactedBehavior();
    
    // TRIVIAL FUNCTIONS
    bool isImpactable() { return true; }
    bool isDynamicallyAdded() { return false; }
private:
    virtual void doBaddieBehavior(Player* player) = 0;
    virtual void doSpecialPauseBehavior() = 0;
    void updatePlayersSharingSquare(std::set<Player*> &updated);
    int m_maxSquaresToMove;
    int m_pauseCounter;
    std::set<Player*>* m_playersSharingSquare;
};

//***************************************************************************//

class Bowser: public Baddie {
public:
    Bowser(int startX, int startY, StudentWorld* world):
    Baddie(10, IID_BOWSER, startX, startY, world) {}
    virtual ~Bowser() {}
private:
    virtual void doBaddieBehavior(Player* player);
    virtual void doSpecialPauseBehavior();
};

class Boo: public Baddie {
public:
    Boo(int startX, int startY, StudentWorld* world):
    Baddie(3, IID_BOO, startX, startY, world) {}
    virtual ~Boo() {}
private:
    virtual void doBaddieBehavior(Player* player);
    virtual void doSpecialPauseBehavior() {}; // Has no special pause behavior...
};

//***************************************************************************//

class Square: public Actor {
public:
    Square(int imageID, int startX, int startY, StudentWorld* world, int depth = 1, int dir = right):
    Actor(imageID, startX, startY, world, depth, dir) {}
    virtual ~Square() {}

    virtual void doSomething();
    virtual void doImpactedBehavior() {};
    
    virtual bool isStatic() { return true; }
    virtual bool isImpactable() { return false; }
    virtual bool isDynamicallyAdded() { return false; }
private:
    virtual void doSquareBehavior(Player* player) = 0;
};

//***************************************************************************//

class Coin: public Square {
public:
    Coin(int coins, int startX, int startY, StudentWorld* world):
    Square(coins >= 0 ? IID_BLUE_COIN_SQUARE : IID_RED_COIN_SQUARE, startX, startY, world), // Makes sprite red automatically if coins is negative
    m_coins(coins) {}
    virtual ~Coin() {}
private:
    virtual void doSquareBehavior(Player* player);
    int m_coins;
};

class Star: public Square {
public:
    Star(int startX, int startY, StudentWorld* world):
    Square(IID_STAR_SQUARE, startX, startY, world) {}

private:
    virtual void doSquareBehavior(Player* player);
};

class Directional: public Square {
public:
    Directional(int dir, int startX, int startY, StudentWorld* world):
    Square(IID_DIR_SQUARE, startX, startY, world, 1, dir) {}
    virtual ~Directional() {}

private:
    virtual void doSquareBehavior(Player* player);
};

class Bank: public Square {
public:
    Bank(int startX, int startY, StudentWorld* world):
    Square(IID_BANK_SQUARE, startX, startY, world) {}
    virtual ~Bank() {}

private:
    virtual void doSquareBehavior(Player* player);
};

class Event: public Square {
public:
    Event(int startX, int startY, StudentWorld* world):
    Square(IID_EVENT_SQUARE, startX, startY, world),
    m_playerThatActivatedSquare(nullptr) {}
    virtual ~Event() {}

private:
    virtual void doSquareBehavior(Player* player);
    Player* m_playerThatActivatedSquare;
};

class Dropping: public Square {
public:
    Dropping(int startX, int startY, StudentWorld* world):
    Square(IID_DROPPING_SQUARE, startX, startY, world) {}
    virtual ~Dropping() {}
    
    virtual bool isDynamicallyAdded() { return true; }
private:
    virtual void doSquareBehavior(Player* player);
};

#endif // ACTOR_H_
