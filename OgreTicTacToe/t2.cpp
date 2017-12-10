#include "t2.h"

//extern as variable is located in another file
extern Ogre::SceneManager *smgr;
extern dSpaceID space;


Board::Board()
{

}


bool Board::checkBoard(){

  bool match = false;
  Player *winningPlayer;

    for(int i = 0; i < 7; i += 3){
      //check for horizontal wins
      Player *horizontalTemp = this->spaces[i].state;
      bool hmatch = true;
      //winningPlayer = horizontalTemp;

      for(int j =0; j<3; j++)
      {

        if(this->spaces[i + j].state == NULL || this->spaces[i + j].state != horizontalTemp)
        {
          hmatch = false;
          break;
        } else {
          winningPlayer = this->spaces[i + j].state;
          //std::cout << winningPlayer << "!!!" << winningPlayer->getName() << "!!!" <<std::endl;
        }

      }

      if(hmatch){
        match = true; break;
      }

      //check for verticle wins

      Player *verticleTemp = this->spaces[i/3].state;
      bool vmatch = true;

      for(int j =0; j<7; j+=3)
      {

        if(this->spaces[i/3 + j].state == NULL || this->spaces[i/3 + j].state != verticleTemp)
        {
          vmatch = false;
          break;
        }else {
          winningPlayer = this->spaces[i + j].state;
        }

      }

      if(vmatch) match = true;


      }

      if(match){
        std::cout<<"Board Won! by: " << winningPlayer->getName();
        //std::cout << " | "<< winningPlayer->getName() << std::endl;
        std::cout << "\n before: " << this->Board::state<<std::endl;
        this->Board::state = winningPlayer;
        std::cout << "\n after: " << this->Board::state->getName() <<std::endl;
        return true;
    } else {
      std::cout<<"no board won" <<std::endl;

      return false;
    }
}

void Board::printBoard()
{

    for(int i =0; i < SPACES_PER_BOARD; i++)
    {
      if(this->spaces[i].state == NULL)
      std::cout << this->spaces[i].state << " |";
      else
      std::cout << this->spaces[i].state->getName() <<  " |";

      if((i+1)%3 == 0)
      std::cout<<std::endl;

    }

}


MainBoard::MainBoard()
{
  std::cout<< "START\n" << std::endl;


  std::cout<<"\nboard spaces: "<< sizeof(this->spaces)/sizeof(this->spaces[0]) << std::endl;
  std::cout<<"inner board spaces: "<< sizeof(this->spaces[0].spaces)/sizeof(this->spaces[0].spaces[0]) << std::endl<< std::endl;

}

void MainBoard::printBoard()
{

  //std::cout<<"TESTING AGAIN: " << this->spaces[0].Board::state <<std::endl;
  for(int j =0; j< SPACES_PER_BOARD; j++)
  {


    this->spaces[j].printBoard();


    if( this->spaces[j].Board::state == NULL)
    {
      std::cout <<"-------------"<< this->spaces[j].Board::state << std::endl;
      std::cout << "NULL\n";

    } else
    {
      std::cout <<"-------------"<< this->spaces[j].Board::state->getName() <<  std::endl;
      std::cout << "NOT NULL\n";
    }

    if((j+1)%3 == 0)
      std::cout<<std::endl;
  }

}

bool MainBoard::makeMove(Player *player, unsigned int j, unsigned int i)
{

  if( j < 9 && i <9 && this->spaces[j].spaces[i].state == NULL )
  {
    this->spaces[j].spaces[i].state = player;
    return true;
  }
  else
  {
    std::cout<<"ILLEGAL MOVE"<<std::endl;
    return false;
  }
}


bool MainBoard::checkBoard()
{

  bool match = false;
  Player *winningPlayer;

  std::cout<< "CHECKING FOR MAIN WIN\n";
    for(int i = 0; i < 7; i += 3){

      bool hmatch = true;

      //check for horizontal wins
      Player *horizontalTemp = this->spaces[i].Board::state;
      winningPlayer = horizontalTemp;

      for(int j =0; j<3; j++)
      {

        if(this->spaces[i + j].Board::state == NULL || this->spaces[i + j].Board::state != horizontalTemp)
        {
          hmatch = false;
          break;
        }

      }

      if(hmatch) match = true;

      //check for verticle wins
      Player *verticleTemp = this->spaces[i/3].Board::state;
      bool vmatch = true;

      for(int j =0; j<7; j+=3)
      {

        if(this->spaces[i/3 + j].Board::state == NULL || this->spaces[i/3 + j].Board::state != verticleTemp)
        {
          vmatch = false;
          break;
        }
      }

      if(vmatch) match = true;


      }

      if(match){
        std::cout<< "---------------\n"<<"GAME WON! by: " << winningPlayer->getName() << "\n---------------"<<std::endl;
        this->Board::state = winningPlayer;
        return true;
    } else {
      std::cout << "----------------\n" << "No Win.. Game On!" << "\n-----------------" <<std::endl;
      return false;
    }
}

Space::Space()
{

  ODEgeomID = dCreateBox(space,1,1,1);//


  dGeomSetPosition (ODEgeomID, 1 ,0, 1 );
  //dGeomSetRotation (spaces[i],R2);

  OgreNode = smgr->getRootSceneNode()->createChildSceneNode();//give it a unique name?
  const dReal *boardPos = dGeomGetPosition(ODEgeomID);
  dReal boardRot[4];  dGeomGetQuaternion(ODEgeomID, boardRot);
  OgreNode->setPosition(boardPos[0],boardPos[1],boardPos[2]);
  OgreNode->setOrientation( boardRot[0], boardRot[1], boardRot[2], boardRot[3]);

  OgreEntity = smgr->createEntity("cube.mesh");//
  OgreEntity->setMaterialName("CSC-30019/Tile1");;
  OgreNode->attachObject(OgreEntity);

  OgreNode->scale(0.01,.04,.01);


}

Space::Space(int x, int z, int y, double margin)
{

  ODEgeomID = dCreateBox(space,1,1,1);//


  dGeomSetPosition (ODEgeomID, (x-1)+((x-1)*margin) ,0, (y-1)+((y-1)*margin) );
  //dGeomSetRotation (spaces[i],R2);

  OgreNode = smgr->getRootSceneNode()->createChildSceneNode();//give it a unique name?
  const dReal *boardPos = dGeomGetPosition(ODEgeomID);
  dReal boardRot[4];  dGeomGetQuaternion(ODEgeomID, boardRot);
  OgreNode->setPosition(boardPos[0],boardPos[1],boardPos[2]);
  OgreNode->setOrientation( boardRot[0], boardRot[1], boardRot[2], boardRot[3]);

  OgreEntity = smgr->createEntity("cube.mesh");//
  OgreEntity->setMaterialName("CSC-30019/Tile1");;
  OgreNode->attachObject(OgreEntity);

  OgreNode->scale(0.01,.04,.01);


}

void Space::setPosition(int x, int z, int y, double margin)
{
  dGeomSetPosition (ODEgeomID, (x-1)+((x-1)*margin) ,0, (y-1)+((y-1)*margin) );
  const dReal *boardPos = dGeomGetPosition(ODEgeomID);

  OgreNode->setPosition(boardPos[0],boardPos[1],boardPos[2]);

  //OgreNode->setPosition(boardPos[0],boardPos[1],boardPos[2]);
  //OgreNode->setOrientation( boardRot[0], boardRot[1], boardRot[2], boardRot[3]);

}
