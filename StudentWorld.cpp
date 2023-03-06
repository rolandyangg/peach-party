#include "StudentWorld.h"
#include "GameConstants.h"
#include <string>
using namespace std;

#include "Actor.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <set>
#include <algorithm>

GameWorld* createStudentWorld(string assetPath)
{
	return new StudentWorld(assetPath);
}

// Students:  Add code to this file, StudentWorld.h, Actor.h, and Actor.cpp

StudentWorld::StudentWorld(string assetPath)
: GameWorld(assetPath)
{
    cerr << "StudentWorld Constructor called" << endl;
    m_board = new Board;
    m_peach = nullptr; // Initialize to nullptr to be safe
    m_yoshi = nullptr;
    m_bank = 0;
}

int StudentWorld::init()
{
    // Initialize data structores TODO: (Should've been done in the constructor already...)
    
    // Load board
    string board_file = assetPath() + "board0" + to_string(getBoardNumber()) + ".txt";
    Board::LoadResult res = m_board->loadBoard(board_file);
    
    // Check if board loaded in correctly
    if (res == Board::load_fail_file_not_found)
    {
        cerr << "ERROR: Could not find data file... " << "['" << board_file << "']" << endl;
        return GWSTATUS_BOARD_ERROR;
    }
    else if (res == Board::load_fail_bad_format)
    {
        cerr << "ERROR: Board improperly formatted... " << "['" << board_file << "']" << endl;
        return GWSTATUS_BOARD_ERROR;
    }
    else if (res == Board::load_success)
        cerr << "SUCCESS: Board loaded successfully! " << "['" << board_file << "']" << endl;
    
    // Populate objects from board into actors
    for (int x = 0; x < BOARD_WIDTH; x++)
    {
        for (int y = 0; y < BOARD_HEIGHT; y++)
        {
            Board::GridEntry ge = m_board->getContentsOf(x, y);
            int sx = x * SPRITE_WIDTH;
            int sy = y * SPRITE_HEIGHT;
            switch (ge) {
                case Board::player:
                    m_peach = new Player(1, sx, sy, this);
                    m_yoshi = new Player(2, sx, sy, this);
                    m_actors.push_back(new Coin(3, sx, sy, this));
                    break;
                case Board::blue_coin_square:
                    m_actors.push_back(new Coin(3, sx, sy, this));
                    break;
                case Board::empty:
                    break;
                case Board::red_coin_square:
                    m_actors.push_back(new Coin(-3, sx, sy, this));
                    break;
                case Board::left_dir_square:
                    m_actors.push_back(new Directional(180, sx, sy, this));
                    break;
                case Board::right_dir_square:
                    m_actors.push_back(new Directional(0, sx, sy, this));
                    break;
                case Board::up_dir_square:
                    m_actors.push_back(new Directional(90, sx, sy, this));
                    break;
                case Board::down_dir_square:
                    m_actors.push_back(new Directional(270, sx, sy, this));
                    break;
                case Board::event_square:
                    m_actors.push_back(new Event(sx, sy, this));
                    break;
                case Board::bank_square:
                    m_actors.push_back(new Bank(sx, sy, this));
                    break;
                case Board::star_square:
                    m_actors.push_back(new Star(sx, sy, this));
                    break;
                case Board::bowser:
                    m_actors.push_back(new Bowser(sx, sy, this));
                    m_actors.push_back(new Coin(3, sx, sy, this));
                    break;
                case Board::boo:
                    m_actors.push_back(new Boo(sx, sy, this));
                    m_actors.push_back(new Coin(3, sx, sy, this));
                    break;
                default:
                    cerr << "ERROR: " << "(" << x << ", " << y << ")" << " none of the above tiles!" << endl;
            }
        }
    }
    
    // Sort m_actors to achieve consistent behaviors due to the order of actions during a tick (ex. always triggering square it lands on after teleporting)
    sortActors();
    
	startCountdownTimer(99);  // this placeholder causes timeout after 5 seconds
    return GWSTATUS_CONTINUE_GAME;
}

int StudentWorld::move()
{
    // Check if game is over and determine winner
    if (timeRemaining() <= 0)
    {
        playSound(SOUND_GAME_FINISHED);
        if (m_peach->getStars() == m_yoshi->getStars())
        {
            // Use coins as tie breaker
            if (m_peach->getCoins() == m_yoshi->getCoins())
            {
                // Pick random winner
                setFinalScore(m_peach->getStars(), m_yoshi->getCoins());
                return (randInt(1, 2) == 1 ? GWSTATUS_PEACH_WON : GWSTATUS_YOSHI_WON);
            }
            else if (m_peach->getCoins() > m_yoshi->getCoins())
            {
                // Peach wins by coins
                setFinalScore(m_peach->getStars(), m_peach->getCoins());
                return GWSTATUS_PEACH_WON;
            }
            else if (m_peach->getCoins() < m_yoshi->getCoins())
            {
                // Yoshi wins by coins
                setFinalScore(m_yoshi->getStars(), m_yoshi->getCoins());
                return GWSTATUS_YOSHI_WON;
            }
        }
        else if (m_peach->getStars() > m_yoshi->getStars())
        {
            // Peach wins by stars
            setFinalScore(m_peach->getStars(), m_peach->getCoins());
            return GWSTATUS_PEACH_WON;
        }
        else if (m_peach->getStars() < m_yoshi->getStars())
        {
            // Yoshi wins by stars
            setFinalScore(m_yoshi->getStars(), m_yoshi->getCoins());
            return GWSTATUS_YOSHI_WON;
        }
    }
    
    // Do something for all actors
    for (int i = 0; i < m_actors.size(); i++)
    {
        if (m_actors[i]->isActive())
            m_actors[i]->doSomething();
    }
    m_peach->doSomething();
    m_yoshi->doSomething();
    
    // Remove dead/inactive objects
    vector<Actor*>::iterator itr = m_actors.begin();
    while (itr != m_actors.end())
    {
        if (!((*itr)->isActive()))
        {
            delete *itr;
            itr = m_actors.erase(itr);
        }
        else
            itr++;
    }
    
    // Update game status text
    setGameStatText(getGameStatusText());
    
    // Game is not over, continue playing
	return GWSTATUS_CONTINUE_GAME;
}

void StudentWorld::cleanUp()
{
    // Set to nullptr to avoid double delete
    cerr << "Framework Clean Up Method called" << endl;
    for (int i = 0; i < m_actors.size(); i++)
    {
        delete m_actors[i];
        m_actors[i] = nullptr;
    }
    m_actors.clear();
    delete m_peach;
    m_peach = nullptr;
    delete m_yoshi;
    m_yoshi = nullptr;
    delete m_board;
    m_board = nullptr;
}

//***************************************************************************//

StudentWorld::~StudentWorld()
{
    cerr << "StudentWorld Destructor called" << endl;
    cleanUp();
}

void StudentWorld::getPlayersOnSquare(std::set<Player*>& playersOnSquare, int x, int y)
{
    // We have to use a set here because there could be multiple players colliding with a square at once and we have to process both of them
    if (m_peach->getX() == x && m_peach->getY() == y)
        playersOnSquare.insert(m_peach);
    if (m_yoshi->getX() == x && m_yoshi->getY() == y)
        playersOnSquare.insert(m_yoshi);
}

void StudentWorld::sortActors()
{
    // TARGET ORDER: [characters, event squares, other squares] (can change later)
    // CHARACTERS
    int tracker = m_actors.size();
    vector<Actor*>::iterator itr = m_actors.begin();
    while (tracker > 0)
    {
        if (!((*itr)->isStatic()))
        {
            m_actors.push_back(*itr);
            itr = m_actors.erase(itr);
        }
        else
            itr++;
        tracker--;
    }
    
    // EVENT SQUARES
    tracker = m_actors.size();
    itr = m_actors.begin();
    while (tracker > 0)
    {
        if (m_board->getContentsOf((*itr)->getX() / SPRITE_WIDTH, (*itr)->getY() / SPRITE_HEIGHT) == Board::event_square)
        {
            m_actors.push_back(*itr);
            itr = m_actors.erase(itr);
        }
        else
            itr++;
        tracker--;
    }
    
    // ALL OTHER SQUARES
    tracker = m_actors.size();
    itr = m_actors.begin();
    while (tracker > 0)
    {
        if ((*itr)->isStatic() && m_board->getContentsOf((*itr)->getX() / SPRITE_WIDTH, (*itr)->getY() / SPRITE_HEIGHT) != Board::event_square)
        {
            m_actors.push_back(*itr);
            itr = m_actors.erase(itr);
        }
        else
            itr++;
        tracker--;
    }
}

Actor* StudentWorld::getImpactableCollidingWith(Actor* actor)
{
    for (int i = 0; i < m_actors.size(); i++)
    {
        // Returns the first impactable actor it can find in the list that's at the spot
        if (m_actors[i]->isImpactable() && isOverlapping(actor, m_actors[i]))
            return m_actors[i];
    }
    return nullptr;
}

Actor* StudentWorld::getRandomSquare(int x, int y)
{
    std::vector<Actor*> squares; // There will always be one square as long as there is a player on the board
    for (int i = 0; i < m_actors.size(); i++)
    {
        if (m_actors[i]->isStatic() && m_actors[i]->getX() != x && m_actors[i]->getY() != y)
            squares.push_back(m_actors[i]);
    }
    int randomSquare = randInt(0, squares.size() - 1);
    return squares[randomSquare];
}

Actor* StudentWorld::getSquareAt(int x, int y)
{
    for (int i = 0; i < m_actors.size(); i++)
        if (m_actors[i]->isStatic() && m_actors[i]->getX() == x && m_actors[i]->getY() == y)
            return m_actors[i];
    cerr << "Error! Could not find square at location (" << x << ", " << y << ")" << endl;
    return nullptr;
}

std::string StudentWorld::getGameStatusText() const
{
    // Ex: "P1 Roll: 3 Stars: 2 $$: 15 VOR | Time: 75 | Bank: 9 | P2 Roll: 0 Stars: 1 $$: 22 VOR"
    ostringstream oss;
    oss << "P1 Roll: " << m_peach->getCurrentRoll();
    oss << " Stars: " << m_peach->getStars();
    oss << " $$: " << m_peach->getCoins();
    if (m_peach->hasVortex())
        oss << " VOR";
    
    oss << " | ";
    oss << "Time: " << timeRemaining();
    oss << " | ";
    oss << "Bank: " << m_bank;
    oss << " | ";
    
    oss << "P2 Roll: " << m_yoshi->getCurrentRoll();
    oss << " Stars: " << m_yoshi->getStars();
    oss << " $$: " << m_yoshi->getCoins();
    if (m_yoshi->hasVortex())
        oss << " VOR";
    
    return oss.str();
}

bool StudentWorld::isOverlapping(Actor* a1, Actor* a2)
{
    // TODO: CHECK TO SEE IF THIS WORKS WITH EDGE CASES
    int minX1 = a1->getX();
    int maxX1 = minX1 + SPRITE_WIDTH - 1;
    int minY1 = a1->getY();
    int maxY1 = minY1 + SPRITE_HEIGHT - 1;

    int minX2 = a2->getX();
    int maxX2 = minX2 + SPRITE_WIDTH - 1;
    int minY2 = a2->getY();
    int maxY2 = minY2 + SPRITE_HEIGHT - 1;
    
    bool xAxisOverlapping = (maxX1 >= minX2) && (maxX2 >= minX1);
    bool yAxisOverlapping = (maxY1 >= minY2) && (maxY2 >= minY1);
    
    return xAxisOverlapping && yAxisOverlapping;
}

bool StudentWorld::isDirectionalSquare(int x, int y)
{
    int gx = x / SPRITE_WIDTH;
    int gy = y / SPRITE_HEIGHT;
    // O(1) check, if not even possibility of being a directional from original board don't bother iterating through all of the actors
    if (!(m_board->getContentsOf(gx, gy) == Board::up_dir_square ||
        m_board->getContentsOf(gx, gy) == Board::down_dir_square ||
        m_board->getContentsOf(gx, gy) == Board::right_dir_square ||
        m_board->getContentsOf(gx, gy) == Board::left_dir_square))
        return false;
    
    // Must check to see if Bowser destroyed original directional square and replaced with dropping square
    for (int i = 0; i < m_actors.size(); i++)
    {
        if (m_actors[i]->getX() == x && m_actors[i]->getY() == y && m_actors[i]->isDynamicallyAdded())
            return false;
    }
    return true;
}
