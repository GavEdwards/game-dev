
#include <ode/ode.h>

// OGRE headers
#include <OGRE/Ogre.h>
#include <OIS/OIS.h>
#include <iostream>

#include <string>


#define SPACES_PER_BOARD 9


class Player
{
public:
  Player(char *_name){this->name = _name;}
  ~Player(){}
  char* getName(){return name;}
  dGeomID ODEgeomID;
  Ogre::SceneNode *OgreNode;
  Ogre::Entity *OgreEntity;
private:
  char *name;
};

class Space
{
public:
  Space();
  Space(int x, int z, int y, double margin);
  ~Space(){ delete state;}
  void setPosition(int x, int z, int y, double margin);
  bool setSpace();
  bool clearSpace();
  Player checkSpace(){ return *state;}
  Player *state = NULL;
  dGeomID ODEgeomID;
  Ogre::SceneNode *OgreNode;
  Ogre::Entity *OgreEntity;
};



class Board
{
public:
  Board();
  //Board(char* meshType);
  ~Board(){delete state;}
  void printBoard();
  bool checkBoard();
  bool updateState(Player *p);
  Space spaces[9];
  Player *state = NULL;
  dGeomID ODEgeomID;
  Ogre::SceneNode *OgreNode;
  Ogre::Entity *OgreEntity;
  int posX, posY, posZ;
};

//create a 'winnable' class that has the state variable for others to inherit


//THERE ARE 10 SPACES PRINTED EACH TIME AS INNER BOARD INHERTITS SPACE
class InnerBoard : public Board, public Space
{
  //will need to use explicit somewhere to avoid inhertience conflicts
  public:
    InnerBoard() : Board() {}//, Space() {}
    ~InnerBoard(){ }

};



//template <class T>
class MainBoard : public Board //inherits from board
{
  public:
    MainBoard();
    ~MainBoard(){}
    void printBoard();
    bool makeMove(Player *player, unsigned int j, unsigned int i);
    bool checkBoard();
    //try and make spaces generic?
    InnerBoard spaces[9];

};
