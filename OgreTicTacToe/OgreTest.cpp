/* buggyogre.cpp
 * -------------
 *
 * This file is the ODE buggy demo, rewritten to use OGRE to display
 * its graphics instead of DrawStuff.
 *
 * The principal changes to the ODE part are just in rearranging the
 * axes so Y is up rather than Z; this is what OGRE expects and ODE
 * is axes-independent.
 *
 * Quaternions (four value descriptions of rotations) are used to pass
 * orientation information from ODE to OGRE. (W, X, Y, Z).
 *
 * Normal 3-vectors are used for position (X, Y, Z).
 *
 */

//local files
#include "t2.h"

//game variables
MainBoard* mb;

Player *p1;
Player *p2;

Player *turn;
unsigned int boardNo, spaceNo;
unsigned int lastMove = 20;

// OGRE variables
Ogre::Root *root;
Ogre::RenderWindow *window;
Ogre::Viewport *vp;
//smgr is also refernced in the other file using extern
Ogre::SceneManager *smgr;

Ogre::Camera *cam;

Ogre::SceneNode *o_ground;
Ogre::SceneNode *buggyNode, *wheel1node, *wheel2node, *wheel3node, *wheel4node;
Ogre::SceneNode *groundBoxNode;
Ogre::SceneNode *spacesNode[9];

// Camera
bool firstPerson = false;
float camx = 0.0, camy = 40.75, camz = 0.0;
float camradius = 2.0, camangle = 0.0;

OIS::Mouse *mouse;
size_t windowHnd = 0;

//Overlay UI
/*
Ogre::Overlay *overlay;
Ogre::OverlayElement *mainPanel;
Ogre::TextAreaOverlayElement* infoText;
*/

//user input manager (show mouse etc)
OIS::InputManager* im;

// Function prototypes (that are defined later)
void initialise();
static void simLoop(int pause);

// some constants
#define LENGTH 0.7	// chassis length
#define WIDTH 0.5	// chassis width
#define HEIGHT 0.2	// chassis height
#define RADIUS 0.18	// wheel radius
#define STARTZ 0.5	// starting height of chassis
#define CMASS 0.5		// chassis mass
#define WMASS 0.2	// wheel mass

// dynamics and collision objects (chassis, 3 wheels, environment)

static dWorldID world;
//dworld space refernces by other file using extern
dSpaceID space;

static dBodyID body[5];
static dJointID joint[4];	// joint[0] is the front wheel
static dJointGroupID contactgroup;
static dGeomID ground;
static dSpaceID car_space;
static dGeomID box[1];
static dGeomID sphere[4];
static dGeomID ground_box;
static dGeomID spaces[9];

// things that the user controls

static dReal speed=0,steer=0;	// user commands


// this is called by dSpaceCollide when two objects in space are
// potentially colliding.

static void nearCallback (void *data, dGeomID o1, dGeomID o2)
{
    int i,n;

    // only collide things with the ground
    int g1 = (o1 == ground || o1 == ground_box);
    int g2 = (o2 == ground || o2 == ground_box);
    if (!(g1 ^ g2)) return;

    const int N = 10;
    dContact contact[N];
    n = dCollide (o1,o2,N,&contact[0].geom,sizeof(dContact));
    if (n > 0) {
        for (i=0; i<n; i++) {
            contact[i].surface.mode = dContactSlip1 | dContactSlip2 |
            dContactSoftERP | dContactSoftCFM | dContactApprox1;
            contact[i].surface.mu = dInfinity;
            contact[i].surface.slip1 = 0.1;
            contact[i].surface.slip2 = 0.1;
            contact[i].surface.soft_erp = 0.5;
            contact[i].surface.soft_cfm = 0.3;
            dJointID c = dJointCreateContact (world,contactgroup,&contact[i]);
            dJointAttach (c,
                          dGeomGetBody(contact[i].geom.g1),
                          dGeomGetBody(contact[i].geom.g2));
        }
    }
}


// simulation loop
static void gameLoop() {

    // Initialise OIS using parameters that leave the mouse pointer visible: (n.b. different for Windows/Mac)
    OIS::ParamList paramList;
    std::ostringstream windowHndStr;
    windowHndStr << windowHnd;
    paramList.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));
    paramList.insert(std::make_pair(std::string("x11_mouse_grab"), std::string("false")));
    paramList.insert(std::make_pair(std::string("x11_mouse_hide"), std::string("false")));
    paramList.insert(std::make_pair(std::string("x11_keyboard_grab"), std::string("false")));
    paramList.insert(std::make_pair(std::string("XAutoRepeatOn"), std::string("true")));

    im = OIS::InputManager::createInputSystem(paramList);

    //set keyboard input and mouse input
    OIS::Keyboard* keyboard = static_cast<OIS::Keyboard*>(im->createInputObject(OIS::OISKeyboard, true));
    mouse = static_cast<OIS::Mouse*>(im->createInputObject(OIS::OISMouse, false));

    // We will use Ogre's hi-res timer to control the game loop
    Ogre::Timer *timer = new Ogre::Timer();

    // Variables for the game loop - see lecture 2
    long currentTime=timer->getMilliseconds(), lastTime=currentTime;
    long frame_length = 20; // milliseconds
    long accumulator = 0;
    int maxUpdatesPerFrame = 10;


    //ray tracing
    Ogre::RaySceneQuery* mSceneQuery;
    mSceneQuery = smgr->createRayQuery(Ogre::Ray());
    mSceneQuery->setSortByDistance(true);

    while( true ) // or until we use the break statement...
    {

        int updatesMade = 0;

        // Required to allow Ogre to pass messages (like Window Closed) to the OS
        Ogre::WindowEventUtilities::messagePump();

        // Collect some input
        keyboard->capture();
        mouse->capture();
        const OIS::MouseState state = mouse->getMouseState();

        if (keyboard->isKeyDown(OIS::KC_ESCAPE)) break;
        if( keyboard->isKeyDown(OIS::KC_A)) { speed += 0.13; }
        if( keyboard->isKeyDown(OIS::KC_Z)) { speed -= 0.13;}
        if( keyboard->isKeyDown(OIS::KC_COMMA)) steer -= 0.15;
        if( keyboard->isKeyDown(OIS::KC_PERIOD)) steer += 0.15;
        if( keyboard->isKeyDown(OIS::KC_SPACE)) { speed = 0; steer = 0; }
        if( keyboard->isKeyDown(OIS::KC_UP)) camradius += 0.5;
        if( keyboard->isKeyDown(OIS::KC_DOWN)) camradius -= 0.5;
        if( keyboard->isKeyDown(OIS::KC_LEFT)) camangle -= M_PI / 30.0;
        if( keyboard->isKeyDown(OIS::KC_RIGHT)) camangle += M_PI / 30.0;

        if( keyboard->isKeyDown(OIS::KC_F)) { firstPerson = true;}
        if( keyboard->isKeyDown(OIS::KC_T)) { camx = 0.0; camy = 40.75; camz = 0.0; camradius = 2.0; camangle = 0.0; firstPerson = false;}


        // Work out how much time has elapsed, add to accumulator
        lastTime = currentTime;
        currentTime=timer->getMilliseconds();
        long elapsedTime = currentTime - lastTime;
        accumulator += elapsedTime;

        // If we're on a fast PC we can "sleep" allowing other programs time on the CPU
        if( accumulator < frame_length ) {
            long frameRemainingMicroSec = 1000*(frame_length-accumulator);
            usleep(frameRemainingMicroSec);
        }

        while( accumulator >= frame_length && updatesMade++ < maxUpdatesPerFrame ) {

            // do game logic, physics etc;
            simLoop(0);
            accumulator -= frame_length;
        }

        // Look at the origin (0,0,0):
        // cam->lookAt(Ogre::Vector3(0,0,0));

        // Or look at the buggy instead?
        if(firstPerson){

          Ogre::Vector3 testpos = buggyNode->getPosition();
          Ogre::Quaternion testrot = buggyNode->_getDerivedOrientation();

          //camx = testpos.x - testrot.w * 2; camy=testpos.y ; camz = testpos.z - testrot.y * 2;
          camx = testpos.x + 3 ; camy=testpos.y ; camz = testpos.z;

          //Ogre::Vector3 lookAt = Ogre::Vector3( -atan2(-testrot.y, testrot.w)*180/M_PI, 0, 0);
          //cam->setDirection(lookAt);
          //cam->lookAt( lookAt );
          cam->lookAt( buggyNode->getPosition() );
          //cam->lookAt(Ogre::Vector3(0,0,0));
        } else {
          // Camera is stuck on a circle on the X-Z plane, camradius from the origin at camangle
          // x = cos(angle) * r; z = sin(angle)*r;
          camx = cosf(camangle) * camradius;
          camz = sinf(camangle) * camradius;
          cam->lookAt( buggyNode->getPosition() );
        }

        cam->setPosition(Ogre::Vector3(camx,camy,camz));

        /*
        *  Ray tracing code for mouse pointer
        */
        //if the mouse is clicked
        if( mouse->getMouseState().buttonDown(OIS::MB_Left) ){
           std::cout<<"MOUSE CLICK\n";

          // Get normalised mouse coordinates
          float sc_x = (float) state.X.abs / vp->getActualWidth();
          float sc_y = (float) state.Y.abs / vp->getActualHeight();

          // Create a ray using the mouse coordinates
          Ogre::Ray pick_ray = cam->getCameraToViewportRay(sc_x, sc_y);

          // Give the scene query the ray
          mSceneQuery->setRay(pick_ray);

          // Execute the scene query
          Ogre::RaySceneQueryResult& result = mSceneQuery->execute();

          // If we had a hit in the ray query, get the entity from the first result
          // (there may be more than one) and set it to a "selected" material:
          if( !result.empty() )
          {
            for(int j = 0; j<9; j++){
              for(int i = 0; i<9; i++){
                if((result[0].movable)->getParentSceneNode() == mb->spaces[j]->spaces[i]->OgreNode) //->movable->getName().compare("Ogre/MO1") == 0)
                {
                  mb->spaces[j]->spaces[i]->OgreNode->showBoundingBox(true);

                  std::cout<< "It is " << turn->getName() <<"'s turn!" << std::endl;

                  if( mb->makeMove(turn,j,i, lastMove) )
                  {
                    //change turn for the next round
                    if(turn == p1){
                    mb->spaces[j]->spaces[i]->OgreEntity->setMaterialName( "CSC-30019/Space1");//"CSC-30019/Tile1-Selected");

                      if( mb->spaces[j]->checkBoard() )
                      {
                          mb->spaces[j]->OgreEntity->setMaterialName( "CSC-30019/Space1");
                          if( mb->checkBoard() ) exit(1);
                      }

                      turn = p2;
                    }
                    else if(turn == p2){
                    mb->spaces[j]->spaces[i]->OgreEntity->setMaterialName( "CSC-30019/Space2");//"CSC-30019/Tile1-Selected");

                      if( mb->spaces[j]->checkBoard() )
                      {
                          mb->spaces[j]->OgreEntity->setMaterialName( "CSC-30019/Space2");
                          if( mb->checkBoard() ) exit(1);
                      }
                      turn = p1;

                    }

                    lastMove = i;

                  }

                  mb->printBoard();

                } else {
                  mb->spaces[j]->spaces[i]->OgreNode->showBoundingBox(false);

                }//end if a board space object
              }//end for loop for space
            }//end for loop for board
          }//end if not empty

          std::cout<< "It is " << turn->getName() <<"'s turn!" << std::endl;

        }//end if click

        // renderOneFrame returns false if it fails, so we will bail in that case.
        if(root->renderOneFrame() == false)
            break;

    }
}

static void setPlayers(char *name1, char *name2)
{
  p1 = new Player(name1);
  p2 = new Player(name2);
  std::cout<< "Created Player 1: " << p1->getName() << std::endl;
  std::cout<< "Created Player 2: " << p2->getName() << std::endl;
  //make player 1 have their turn first
  turn = p1;

}


// This is the old DrawStuff simLoop - hacked to do Ogre stuff instead!
// (And now it's called from our game loop, rather than DrawStuff's)
static void simLoop (int pause)
{
    int i;

    if (!pause) {
      for(int i = 0; i < 2; i++){
        // motor
        dJointSetHinge2Param (joint[i],dParamVel2,-speed);
        dJointSetHinge2Param (joint[i],dParamFMax2,0.1);

        // steering
        dReal v = steer - dJointGetHinge2Angle1 (joint[i]);
        if (v > 0.1) v = 0.1;
        if (v < -0.1) v = -0.1;
        v *= 10.0;
        dJointSetHinge2Param (joint[i],dParamVel,v);
        dJointSetHinge2Param (joint[i],dParamFMax,0.2);
        dJointSetHinge2Param (joint[i],dParamLoStop,-0.75);
        dJointSetHinge2Param (joint[i],dParamHiStop,0.75);
        dJointSetHinge2Param (joint[i],dParamFudgeFactor,0.1);
      }
        dSpaceCollide (space,0,&nearCallback);
        dWorldStep (world,0.05);

        // remove all contact joints
        dJointGroupEmpty (contactgroup);
    }

    // GET POSITIONS OF EVERYTHING FROM PHYSICS, UPDATE GRAPHICS
    const dReal *boxpos = dBodyGetPosition(body[0]);
    const dReal *boxrot = dBodyGetQuaternion(body[0]);

    buggyNode->setPosition( boxpos[0], boxpos[1], boxpos[2] );
    buggyNode->setOrientation( boxrot[0], boxrot[1], boxrot[2], boxrot[3]);

    const dReal *gbpos = dGeomGetPosition(ground_box);
    dReal gbrot[4];  dGeomGetQuaternion(ground_box,gbrot);

    groundBoxNode->setPosition(gbpos[0],gbpos[1],gbpos[2]);
    groundBoxNode->setOrientation( gbrot[0], gbrot[1], gbrot[2], gbrot[3]);

    const dReal *w1pos = dBodyGetPosition(body[1]);
    const dReal *w1rot = dBodyGetQuaternion(body[1]);

    wheel1node->setPosition(w1pos[0],w1pos[1],w1pos[2]);
    wheel1node->setOrientation(w1rot[0],w1rot[1],w1rot[2],w1rot[3]);

    const dReal *w2pos = dBodyGetPosition(body[2]);
    const dReal *w2rot = dBodyGetQuaternion(body[2]);

    wheel2node->setPosition(w2pos[0], w2pos[1], w2pos[2]);
    wheel2node->setOrientation(w2rot[0], w2rot[1], w2rot[2], w2rot[3]);

    const dReal *w3pos = dBodyGetPosition(body[3]);
    const dReal *w3rot = dBodyGetQuaternion(body[3]);

    wheel3node->setPosition(w3pos[0], w3pos[1], w3pos[2]);
    wheel3node->setOrientation(w3rot[0], w3rot[1], w3rot[2], w3rot[3]);

    const dReal *w4pos = dBodyGetPosition(body[4]);
    const dReal *w4rot = dBodyGetQuaternion(body[4]);

    wheel4node->setPosition(w4pos[0], w4pos[1], w4pos[2]);
    wheel4node->setOrientation(w4rot[0], w4rot[1], w4rot[2], w4rot[3]);

}


int main (int argc, char **argv)
{
    int i;
    dMass m;

    // OGRE Initialisation
    initialise();

    // BEGIN old ODE code which does the buggy simulation
    // create world
    dInitODE2(0);
    world = dWorldCreate();
    space = dHashSpaceCreate (0);
    contactgroup = dJointGroupCreate (0);
    dWorldSetGravity (world,0,-0.5,0);
    ground = dCreatePlane (space,0,1,0,0);

    // chassis body
    body[0] = dBodyCreate (world);
    dBodySetPosition (body[0],0,STARTZ,0);
    dMassSetBox (&m,1,LENGTH,HEIGHT,WIDTH);
    dMassAdjust (&m,CMASS);
    dBodySetMass (body[0],&m);
    box[0] = dCreateBox (0,LENGTH,HEIGHT,WIDTH);
    dGeomSetBody (box[0],body[0]);

    // wheel bodies
    for (i=1; i<=4; i++) {
        body[i] = dBodyCreate (world);
        dQuaternion q;
        dQFromAxisAndAngle (q,1,0,0,M_PI*0.5);
        dBodySetQuaternion (body[i],q);
        dMassSetSphere (&m,1,RADIUS);
        dMassAdjust (&m,WMASS);
        dBodySetMass (body[i],&m);
        sphere[i-1] = dCreateSphere (0,RADIUS);
        dGeomSetBody (sphere[i-1],body[i]);
    }
    dBodySetPosition (body[1],-0.5*LENGTH,STARTZ-HEIGHT*0.5,WIDTH*0.5);
    dBodySetPosition (body[2],-0.5*LENGTH,STARTZ-HEIGHT*0.5,-WIDTH*0.5);
    dBodySetPosition (body[3],0.5*LENGTH, STARTZ-HEIGHT*0.5,WIDTH*0.5);
    dBodySetPosition (body[4],0.5*LENGTH, STARTZ-HEIGHT*0.5,-WIDTH*0.5);

    // front and back wheel hinges
    for (i=0; i<4; i++) {
        joint[i] = dJointCreateHinge2 (world,0);
        dJointAttach (joint[i],body[0],body[i+1]);
        const dReal *a = dBodyGetPosition (body[i+1]);
        dJointSetHinge2Anchor (joint[i],a[0],a[1],a[2]);
        dJointSetHinge2Axis1 (joint[i],0,1,0);
        dJointSetHinge2Axis2 (joint[i],0,0,1);
    }

    // set joint suspension
    for (i=0; i<4; i++) {
        dJointSetHinge2Param (joint[i],dParamSuspensionERP,0.4);
        dJointSetHinge2Param (joint[i],dParamSuspensionCFM,0.8);
    }

    // lock back wheels along the steering axis
    for (i=2; i<4; i++) {
        // set stops to make sure wheels always stay in alignment
        dJointSetHinge2Param (joint[i],dParamLoStop,0);
        dJointSetHinge2Param (joint[i],dParamHiStop,0);

    }

    // create car space and add it to the top level space
    car_space = dSimpleSpaceCreate (space);
    dSpaceSetCleanup (car_space,0);
    dSpaceAdd (car_space,box[0]);
    dSpaceAdd (car_space,sphere[0]);
    dSpaceAdd (car_space,sphere[1]);
    dSpaceAdd (car_space,sphere[2]);
    dSpaceAdd (car_space,sphere[3]);

    // environment
    ground_box = dCreateBox (space,2,1,1.5);
    dMatrix3 R;
    dRFromAxisAndAngle (R,0,0,1,-0.15);
    dGeomSetPosition (ground_box,-2,-0.34,0);
    dGeomSetRotation (ground_box,R);


    //create the board
    //game initialise
    mb = new MainBoard();

    //mb->setPosition( 0, 0, 0, 0);
/*
    int countT = 0;
    double marginT = 2;

    for(int f =0; f < 9; f++){

      int xT = f%3;
      int yT = countT;

      if(xT==2) countT++;


      int count = 0;
      double margin = 2;

      //Board::setPosition
      //mb->spaces[f].setPosition( xT, 0, yT, marginT);

      for(int i =0; i< 9 ; i++){

        int x = i%3;
        int y = count;

        if(x==2) count++;

        x = x + xT;
        y = y + yT;

        mb->spaces[f].spaces[i].setPosition( x+((xT)+(xT-2)*marginT), 0, y +((yT) + (yT-2) *marginT), margin);

      }
    }
*/

    // Set the scene
    window->getCustomAttribute("WINDOW", &windowHnd);
    cam = smgr->createCamera("Camera1");
    camx = cosf(camangle) * camradius; camz = sinf(camangle) * camradius;
    cam->setPosition(Ogre::Vector3(camx,camy,camz));
    cam->lookAt(Ogre::Vector3(0,0,0));
    cam->setNearClipDistance(1);
    vp = window->addViewport(cam); //use for ray tracing from camera view
    cam->setAspectRatio(Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()) );

    // SKYBOX
    smgr->setSkyBox(true, "CSC-30019/TrippySkyBox");


    // Nodes for the buggy and the angled box in the ground
    buggyNode = smgr->getRootSceneNode()->createChildSceneNode("buggynode");
    groundBoxNode = smgr->getRootSceneNode()->createChildSceneNode("groundboxnode");


    // Entities for the various bits we are displaying
    Ogre::Entity *ent = smgr->createEntity("buggy", "cube.mesh");
    Ogre::Entity *gbent = smgr->createEntity("groundbox", "cube.mesh");

    //char ids[9] = {'1','2','3','4','5','6','7','8','9'};
    Ogre::Entity *spacest[9];
    for(int i =0; i<9; i++){
       spacest[i]= smgr->createEntity("cube.mesh");
     }

    Ogre::Entity *wheel1 = smgr->createEntity("wheel1", "sphere.mesh");
    Ogre::Entity *wheel2 = smgr->createEntity("wheel2", "sphere.mesh");
    Ogre::Entity *wheel3 = smgr->createEntity("wheel3", "sphere.mesh");
    Ogre::Entity *wheel4 = smgr->createEntity("wheel4", "sphere.mesh");

    //gbent->setMaterial(mp);
    buggyNode->attachObject(ent);// buggyNode->attachObject(cam);
    buggyNode->scale(0.01*(LENGTH),0.01*(HEIGHT),0.01*(WIDTH));

    wheel1node = smgr->getRootSceneNode()->createChildSceneNode("wheel1node");
    wheel1node->attachObject(wheel1);
    wheel1node->scale(0.002,0.0005,0.002);
    wheel1node->setPosition(0.5*LENGTH,STARTZ-HEIGHT*0.5,0);
    wheel2node = smgr->getRootSceneNode()->createChildSceneNode("wheel2node");
    wheel2node->attachObject(wheel2);
    wheel2node->scale(0.002,0.0005,0.002);
    wheel2node->setPosition(-0.5*LENGTH,STARTZ-HEIGHT*0.5,0);
    wheel3node = smgr->getRootSceneNode()->createChildSceneNode("wheel3node");
    wheel3node->attachObject(wheel3);
    wheel3node->scale(0.002,0.0005,0.002);
    wheel3node->setPosition(-0.5*LENGTH,STARTZ-HEIGHT*0.5,0);
    wheel4node = smgr->getRootSceneNode()->createChildSceneNode("wheel4node");
    wheel4node->attachObject(wheel4);
    wheel4node->scale(0.002,0.0005,0.002);
    wheel4node->setPosition(-0.5*LENGTH,STARTZ-HEIGHT*0.5,0);


    ent->setMaterialName("CSC-30019/Tile1");

    gbent->setMaterialName("CSC-30019/Tile2");
    groundBoxNode->attachObject(gbent);
    groundBoxNode->scale(0.02,.01,.015);

    //set spaces texture


    // Set the ambient lighting in the scene
    smgr->setAmbientLight(Ogre::ColourValue(0.3,0.3,0.4));

    // Directional lighting so we get nice shadows
    Ogre:: Light *dirLight = smgr->createLight("dir_light");
    dirLight->setType(Ogre::Light::LT_DIRECTIONAL );
    dirLight->setDirection(Ogre::Vector3(0.707,-.707,0));
    dirLight->setDiffuseColour(1.0,1.0,1.0);

    // Shadows have to be enabled though
    smgr->setShadowTechnique(Ogre::SHADOWTYPE_STENCIL_ADDITIVE);


    //WRAP IN TRY CATCH TO VALIDATE USER INPUT
      char n1[15], n2[15];
      std::cout<<"Enter player 1 name: ";
      std::cin >> n1;
      std::cout<<"Enter player 2 name: ";
      std::cin>> n2;
      setPlayers(n1, n2);

    // render a frame for good measure
    root->renderOneFrame();
    // END OGRE Initialisation

    // run simulation (using our own game loop)
    gameLoop();

    // tidy up
    dGeomDestroy (box[0]);
    dGeomDestroy (sphere[0]);
    dGeomDestroy (sphere[1]);
    dGeomDestroy (sphere[2]);
    dJointGroupDestroy (contactgroup);
    dSpaceDestroy (space);
    dWorldDestroy (world);
    dCloseODE();
    return 0;
}



void initialise()
{

    std::string resourcePath;
    resourcePath = "";

    root = new Ogre::Root(resourcePath + "plugins.cfg", resourcePath + "ogre.cfg", "Ogre.log");

    if (!root->showConfigDialog())
        exit(-1);

    Ogre::ConfigFile cf;
    cf.load(resourcePath + "resources.cfg");

    // Go through all sections & settings in the file
    Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

    Ogre::String secName, typeName, archName;
    while (seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            typeName = i->first;
            archName = i->second;

            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
        }
    }

    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

    window = root->initialise(true);

    std::cout << "-------- \n"<< smgr << std::endl;
    smgr = root->createSceneManager(Ogre::ST_GENERIC);
    std::cout << "-------- \n"<< smgr << std::endl;

}
