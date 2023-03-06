#include "Actor.h"
#include "StudentWorld.h"

// Students:  Add code to this file, Actor.h, StudentWorld.h, and StudentWorld.cpp
//#include "GameWorld.h" // TODO: CHECK IF HAVE TO ACTUALLY INCLUDE THIS...
#include <iostream>
#include <string>
using namespace std;

// "up", "left", "down", "right" constants are located in GraphObject.h

///////////////////////////////////////////////////////////////////////////
//  CHARACTER
///////////////////////////////////////////////////////////////////////////

void Character::swapPhysicalState(Character* otherCharacter)
{
    // Swap x, y coordinates
    int oldX = getX();
    int oldY = getY();
    moveTo(otherCharacter->getX(), otherCharacter->getY());
    otherCharacter->moveTo(oldX, oldY);
    
    // Swap ticks
    int oldTicks = getTicks();
    setTicks(otherCharacter->getTicks());
    otherCharacter->setTicks(oldTicks);
    
    // Swap walk direction
    int oldWalkDirection = getWalkDirection();
    setWalkDirection(otherCharacter->getWalkDirection());
    otherCharacter->setWalkDirection(oldWalkDirection);
    
    // Swap sprite direction
    int oldSpriteDirection = getDirection();
    setDirection(otherCharacter->getDirection());
    otherCharacter->setDirection(oldSpriteDirection);
    
    // Swap roll/walk state
    bool oldState = getState();
    setState(otherCharacter->getState());
    otherCharacter->setState(oldState);
}

void Character::updateSpriteDirection()
{
    if (m_walkDirection == left)
        setDirection(180);
    else
        setDirection(0);
}

void Character::handleCornerTurn()
{
    // Going left or right prefer upwards
    if (m_walkDirection == left || m_walkDirection == right)
    {
        if (canMoveInDirection(up))
            m_walkDirection = up;
        else
            m_walkDirection = down;
    }
    // Going up or down prefer right
    else if (m_walkDirection == up || m_walkDirection == down)
    {
        if (canMoveInDirection(right))
            m_walkDirection = right;
        else
            m_walkDirection = left;
    }
}

int Character::getRandomDirection()
{
    int randDirection = randInt(0, 3) * 90;
    while (!canMoveInDirection(randDirection))
        randDirection = randInt(0, 3) * 90;
    return randDirection;
}

bool Character::canMoveInDirection(int dir)
{
    int predictedX, predictedY;
    getPositionInThisDirection(dir, SPRITE_WIDTH, predictedX, predictedY);
    return !(getWorld()->isEmptySquare(predictedX, predictedY));
}

bool Character::isAtFork()
{
    int counter = 0;
    for (int i = 0; i < 360; i += 90)
        if (canMoveInDirection(i))
            counter++;
    return (counter - 1) >= 2;
}

//***************************************************************************//

///////////////////////////////////////////////////////////////////////////
//  PLAYER
///////////////////////////////////////////////////////////////////////////

void Player::doSomething()
{
    if (getState() == WAITING_TO_ROLL)
    {
        if (getWalkDirection() == invalid_direction)
        {
            // Choose a random direction player can move in
            setWalkDirection(getRandomDirection());
            updateSpriteDirection();
        }
        int action = getWorld()->getAction(m_playerNumber);
        if (action == ACTION_ROLL)
        {
            // Roll the dice
            int die_roll = randInt(1, 10);
            setTicks(die_roll * 8);
            setState(WALKING);
            cerr << "PLAYER " << m_playerNumber << " rolled a: " << die_roll << endl;
        }
        else if (action == ACTION_FIRE)
        {
            if (hasVortex())
            {
                // Make a new vortex
                int x, y;
                getPositionInThisDirection(getWalkDirection(), SPRITE_WIDTH, x, y);
                getWorld()->addActor(new Vortex(getWalkDirection(), x, y, getWorld()));
                getWorld()->playSound(SOUND_PLAYER_FIRE);
                setVortex(false);
                cerr << "PLAYER " << m_playerNumber << " shot a vortex" << endl;
            }
        }

        setNew(false);
        return;
    }
    
    if (getState() == WALKING)
    {
        setNew(true);
        // Update direction (For efficiency only check when it reaches the next square)
        if (getX() % SPRITE_WIDTH == 0 && getY() % SPRITE_WIDTH == 0)
        {
            if (getWorld()->isDirectionalSquare(getX(), getY()))
            {
                // Update what the sprite looks like now that the Directional square changed the player's walk direction
                updateSpriteDirection();
                cerr << "PLAYER " << m_playerNumber << " was walking and hit a directional square! Currently going: " << getWalkDirection() << endl;
            }
            else if (isAtFork())
            {
                int action = getWorld()->getAction(m_playerNumber);
                if ( (action == ACTION_LEFT || action == ACTION_RIGHT || action == ACTION_DOWN || action == ACTION_UP) &&
                    isLegalDirectionChange(getDirectionFromAction(action)) )
                {
                    setWalkDirection(getDirectionFromAction(action));
                    updateSpriteDirection();
                    cerr << "PLAYER " << m_playerNumber << " at a fork chose to go " << getWalkDirection() << endl;
                }
                else
                    return;
            }
            else if (!canMoveInDirection(getWalkDirection()))
            {
                handleCornerTurn();
                updateSpriteDirection();
            }
        }

        moveAtAngle(getWalkDirection(), 2);
        setTicks(getTicks() - 1);
        if (getTicks() == 0)
            setState(WAITING_TO_ROLL);
    }
}

int Player::getDirectionFromAction(int playerAction)
{
    switch (playerAction)
    {
        case ACTION_RIGHT:
            return 0;
        case ACTION_UP:
            return 90;
        case ACTION_LEFT:
            return 180;
        case ACTION_DOWN:
            return 270;
        default:
            cerr << "ERROR: Invalid call to getDirectionFromAction! " << playerAction << endl;
    }
    return -1;
}

//***************************************************************************//

///////////////////////////////////////////////////////////////////////////
//  VORTEX
///////////////////////////////////////////////////////////////////////////

void Vortex::doSomething()
{
    if (!isActive())
        return;
    
    moveForward(2);
    
    // Check out of bounds
    if (getX() < 0 || getX() >= VIEW_WIDTH || getY() < 0 || getY() >= VIEW_HEIGHT)
        setActive(false);
    
    Actor* impactedActor = getWorld()->getImpactableCollidingWith(this);
    if (impactedActor != nullptr)
    {
        impactedActor->doImpactedBehavior();
        setActive(false);
        getWorld()->playSound(SOUND_HIT_BY_VORTEX);
    }
}

//***************************************************************************//

///////////////////////////////////////////////////////////////////////////
//  BADDIE
///////////////////////////////////////////////////////////////////////////

void Baddie::doSomething()
{
    if (getState() == PAUSED)
    {
        // Do Baddie's action on whatever players it is sharing the square with
        // TODO: Look into moving this into another function and factoring out checking whether player state is in waiting to roll
        set<Player*> playersOnSquare;
        getWorld()->getPlayersOnSquare(playersOnSquare, getX(), getY());
        
        for (Player* player : playersOnSquare)
        {
            if (m_playersSharingSquare->find(player) == m_playersSharingSquare->end())
            {
                m_playersSharingSquare->insert(player);
                doBaddieBehavior(player);
            }
        }
        
        // Process if a player left the square
        updatePlayersSharingSquare(playersOnSquare);
        
        m_pauseCounter--;
        if (m_pauseCounter == 0)
        {
            
            int squares_to_move = randInt(1, m_maxSquaresToMove);
            setTicks(squares_to_move * 8);
            
            // Choose random direction to move in, make sure it can move onto it
            int randDirection = getRandomDirection();
            while (!canMoveInDirection(randDirection))
                randDirection = getRandomDirection();
            setWalkDirection(randDirection);
            updateSpriteDirection();
            setState(WALKING);
        }
    }

    if (getState() == WALKING)
    {
        if (getX() % SPRITE_WIDTH == 0 && getY() % SPRITE_WIDTH == 0)
        {
            if (isAtFork())
            {
                int randDirection = getRandomDirection();
                while (!canMoveInDirection(randDirection))
                    randDirection = getRandomDirection();
                setWalkDirection(randDirection);
            }
            else if (!canMoveInDirection(getWalkDirection()))
                handleCornerTurn();
            
            updateSpriteDirection();
        }

        moveAtAngle(getWalkDirection(), 2);
        setTicks(getTicks() - 1);
        if (getTicks() == 0)
        {
            setState(PAUSED);
            m_pauseCounter = 180;
            doSpecialPauseBehavior();
        }
    }
}

Baddie::~Baddie()
{
    // We don't have to delete the items it is pointing to because it's going to be a Player which is deleted later in StudentWorld
    cerr << "Baddie Deconstructor Called" << endl;
    m_playersSharingSquare->clear();
    delete m_playersSharingSquare;
}

void Baddie::doImpactedBehavior()
{
    // Teleport to another random square on board and reset it to starting state
    Actor* randomSquare = getWorld()->getRandomSquare(getX(), getY());
    moveTo(randomSquare->getX(), randomSquare->getY());
    setWalkDirection(right);
    setDirection(0);
    setState(PAUSED);
    m_pauseCounter = 180;
}

void Baddie::updatePlayersSharingSquare(set<Player*> &updated)
{
    set<Player*>::iterator itr;
    // If the old version of players, a player is not in the new version of players on the square
    for (itr = m_playersSharingSquare->begin(); itr != m_playersSharingSquare->end(); )
    {
        if (updated.find(*itr) == updated.end())
            itr = m_playersSharingSquare->erase(itr);
        else
            itr++;
    }
}

//***************************************************************************//

///////////////////////////////////////////////////////////////////////////
//  BOWSER
///////////////////////////////////////////////////////////////////////////

void Bowser::doBaddieBehavior(Player* player)
{
    if (player->getState() == WAITING_TO_ROLL)
    {
        if (randInt(1, 2) == 1)
        {
            // 50% chance Make player lose all their coins and stars
            player->setStars(0);
            player->setCoins(0);
            getWorld()->playSound(SOUND_BOWSER_ACTIVATE);
            cerr << "Bowser just made a player lose all their coins!" << endl;
        }
    }
}

void Bowser::doSpecialPauseBehavior()
{
    if (randInt(1, 4) == 1)
    {
        // 25% chance deactivate square underneath and set to dropping square
        getWorld()->getSquareAt(getX(), getY())->setActive(false);
        getWorld()->addActor(new Dropping(getX(), getY(), getWorld()));
        getWorld()->playSound(SOUND_DROPPING_SQUARE_CREATED);
        cerr << "Bowser created a dropping square at: (" << getX() << ", " << getY() << ")" << endl;
    }
}

//***************************************************************************//

///////////////////////////////////////////////////////////////////////////
//  BOO
///////////////////////////////////////////////////////////////////////////

void Boo::doBaddieBehavior(Player* player)
{
    if (player->getState() == WAITING_TO_ROLL)
    {
        Player* otherPlayer = getWorld()->getOtherPlayer(player);
        if (randInt(1, 2) == 1)
        {
            // Swap coins with other player
            int oldCoins = player->getCoins();
            player->setCoins(otherPlayer->getCoins());
            otherPlayer->setCoins(oldCoins);
            cerr << "Boo swapped the players COINS!" << endl;
        }
        else
        {
            // Swap stars with other player
            int oldStars = player->getStars();
            player->setStars(otherPlayer->getStars());
            otherPlayer->setStars(oldStars);
            cerr << "Boo swapped the players STARS!" << endl;
        }
        getWorld()->playSound(SOUND_BOO_ACTIVATE);
    }
}

//***************************************************************************//

///////////////////////////////////////////////////////////////////////////
//  SQUARE
///////////////////////////////////////////////////////////////////////////

void Square::doSomething()
{
    // Process new players that landed on the square
    set<Player*> playersOnSquare;
    getWorld()->getPlayersOnSquare(playersOnSquare, getX(), getY());
    
    for (Player* player : playersOnSquare)
    {
        // NOTE: A player is considered no longer considered "new" if it has been in place for more than a single tick (in this case 0 is the first tick, 1 is the second tick)
        if (player->isNew())
        {
            if (isActive() == false) // Check if active to avoid having Dropping square and other square effects run at same time
                return;
            doSquareBehavior(player);
        }
    }
}

//***************************************************************************//

///////////////////////////////////////////////////////////////////////////
//  COIN
///////////////////////////////////////////////////////////////////////////

void Coin::doSquareBehavior(Player* player)
{
    // Player landed on the square
    if (player->getState() == WAITING_TO_ROLL)
    {
        // Change amount of coins player has
        player->setCoins(player->getCoins() + m_coins);
        if (player->getCoins() < 0)
            player->setCoins(0);
        cerr << "Changed a player's coins by " << m_coins << endl;

        // Play the appropriate sound
        if (m_coins >= 0)
            getWorld()->playSound(SOUND_GIVE_COIN);
        else
            getWorld()->playSound(SOUND_TAKE_COIN);
    }
}

//***************************************************************************//

///////////////////////////////////////////////////////////////////////////
//  STAR
///////////////////////////////////////////////////////////////////////////

void Star::doSquareBehavior(Player* player)
{
    if (player->getCoins() < 20)
        return;
    else
    {
        // The player has enough coins
        player->setCoins(player->getCoins() - 20);
        player->setStars(player->getStars() + 1);
        getWorld()->playSound(SOUND_GIVE_STAR);
    }
}

//***************************************************************************//

///////////////////////////////////////////////////////////////////////////
//  DIRECTIONAL
///////////////////////////////////////////////////////////////////////////

void Directional::doSquareBehavior(Player* player)
{
    player->setWalkDirection(getDirection());
    cerr << "Directional Square changing player's walk direction to: " << getDirection() << endl;
}

//***************************************************************************//

///////////////////////////////////////////////////////////////////////////
//  BANK
///////////////////////////////////////////////////////////////////////////

void Bank::doSquareBehavior(Player* player)
{
    if (player->getState() == WAITING_TO_ROLL)
    {
        // Player landed on bank, give them everything in the bank
        player->setCoins(player->getCoins() + getWorld()->getBank());
        cerr << "Kaching!!! A player cashed out and won " << getWorld()->getBank() << " from the bank!!!" << endl;
        getWorld()->setBank(0);
        getWorld()->playSound(SOUND_WITHDRAW_BANK);
    }
    else if (player->getState() == WALKING)
    {
        // Player moved over bank, take away coins from them
        int deductedCoins = 5;
        player->setCoins(player->getCoins() - deductedCoins);
        if (player->getCoins() < 0)
        {
            // Handle deducting coins when player doesn't have enough coins
            deductedCoins -= player->getCoins() * -1;
            player->setCoins(0);
        }
        getWorld()->setBank(getWorld()->getBank() + deductedCoins);
        getWorld()->playSound(SOUND_DEPOSIT_BANK);
        cerr << "A player has to pay " << deductedCoins << " in taxes to the bank..." << endl;
    }
}

//***************************************************************************//

///////////////////////////////////////////////////////////////////////////
//  EVENT
///////////////////////////////////////////////////////////////////////////

void Event::doSquareBehavior(Player* player) {
    // Do checks to make sure not to double activate event square when player positions swapped (might run into some errors if two players land on an event square at the same time...)
    if (m_playerThatActivatedSquare == nullptr)
        m_playerThatActivatedSquare = player;
    else if (m_playerThatActivatedSquare == getWorld()->getOtherPlayer(player))
    {
        cerr << "Preventing double activation on event square..." << endl;
        m_playerThatActivatedSquare = nullptr;
        return;
    }
    
    if (player->getState() == WAITING_TO_ROLL)
    {
        int choice = randInt(1, 3);
        if (choice == 1)
        {
            // Teleport to another random square on board
            Actor* randomSquare = getWorld()->getRandomSquare(getX(), getY());
            player->moveTo(randomSquare->getX(), randomSquare->getY());
            player->setWalkDirection(invalid_direction);
            player->setNew(true);
            getWorld()->playSound(SOUND_PLAYER_TELEPORT);
            cerr << "Event Case 1: Player randomly teleported to " << "(" << randomSquare->getX() << ", " << randomSquare->getY() << ")" << endl;
        }
        else if (choice == 2)
        {
            // Swap player positions
            player->swapPhysicalState(getWorld()->getOtherPlayer(player));
            
            // "Player that triggered the swap SHOULD trigger the square it lands on"
            player->setNew(true);
            
            getWorld()->playSound(SOUND_PLAYER_TELEPORT);
            /**
             TODO: NOTE: After the players have been swapped, the Event Square must NOT activate on the other player who just had its position swapped onto the Event Square. (Possible bug... attempt playerActivated)
             */
            cerr << "Event Case 2: Players Swapped Positions" << endl;
        }
        else
        {
            // Give vortex
            player->setVortex(true);
            getWorld()->playSound(SOUND_GIVE_VORTEX);
            cerr << "Event Case 3: Player given Vortex" << endl;
        }
    }
}

//***************************************************************************//

///////////////////////////////////////////////////////////////////////////
//  DROPPING
///////////////////////////////////////////////////////////////////////////

void Dropping::doSquareBehavior(Player* player) {
    if (player->getState() == WAITING_TO_ROLL)
    {
        int choice = randInt(0, 1);
        if (choice == 0)
        {
            // Deduct 10 coins
            player->setCoins(player->getCoins() - 10);
            if (player->getCoins() < 0)
                player->setCoins(0);
            cerr << "The dropping square deducted 10 coins..." << endl;
        }
        else
        {
            // Deduct 1 star
            if (player->getStars() >= 1)
                player->setStars(player->getStars() - 1);
            cerr << "A dropping square deducted 1 star..." << endl;
        }
        getWorld()->playSound(SOUND_DROPPING_SQUARE_ACTIVATE);
    }
}
