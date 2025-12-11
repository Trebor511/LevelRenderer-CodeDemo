//RenderManager
#include <d3dcompiler.h>	// required for compiling shaders on the fly, consider pre-compiling instead

#include "load_object_oriented.h"
#pragma comment(lib, "d3dcompiler.lib") 




// Creation, Rendering & Cleanup
class RenderManager
{
	std::chrono::steady_clock::time_point lastUpdate; //Timer variables
	float deltaTime;

	GW::MATH::GMatrix proxyMat; // proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GDirectX11Surface d3d;

	GW::INPUT::GInput input;  //Support for Keyboard Input or Controller (Controller not implimented)
	GW::INPUT::GController controller;

	GW::MATH::GMATRIXF world; //Declaration of World, View, and Projection Matrix
	GW::MATH::GMATRIXF view;
	GW::MATH::GMATRIXF pers;

	GW::MATH::GMATRIXF currView; // Used in Specular Reflection

	Level_Objects theLevel; //Level Objects

	GW::SYSTEM::GLog log; // handy for logging any messages/warning/errors

public:
	RenderManager(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX11Surface _d3d)
	{
		win = _win;
		d3d = _d3d;

		// begin loading level
		log.Create("../LevelLoaderLog.txt");
		log.EnableConsoleLogging(true); // mirror output to the console
		log.Log("Start Program.");

		theLevel.LoadLevel("../Levels/GameLevelOne.txt", "../Models", log); //Loads Level in Object Oriented method

		proxyMat.Create();		//Create Proxy and Initialize Matrices
		CreateMatricies(_d3d);

		input.Create(_win); //Initialize int=puts
		controller.Create();

		theLevel.UploadLevelToGPU(_d3d, world, view, pers); //Send Initalized data to GPU
	}

	//constructor helper functions

	void Render()
	{

		theLevel.RenderLevel(d3d, view, currView); //Renders the Inital Level on Construction

	}
	void UpdateCamera()
	{
		proxyMat.InverseF(view, view); //Inverse the view matrix

		float yChange = 0.0f; //Initialize Variables to contain the change over button presses and mouse movement
		float zChange = 0.0f;
		float xChange = 0.0f;
		float xDelta = 0.0f;
		float yDelta = 0.0f;
		float spaceC = 0.0f;					
		float shiftC = 0.0f;
		float wC = 0.0f;
		float sC = 0.0f;
		float aC = 0.0f;
		float dC = 0.0f;

		const float cSpeed = 3.0; //Camera Speed

		GW::MATH::GMATRIXF zero;
		proxyMat.IdentityF(zero);

		unsigned int height = 0.0f; //Variables to contain the hight and width of the window and aspect ratio
		unsigned int width = 0.0f; 
		float aRatio = 0.0f;

		win.GetClientHeight(height); // Collects Hight, Width, and Aspect Ratio
		win.GetClientWidth(width);
		d3d.GetAspectRatio(aRatio);

		input.GetState(G_KEY_SPACE, spaceC);	//Collects Input from the Keyboard
		input.GetState(G_KEY_LEFTSHIFT, shiftC);
		input.GetState(G_KEY_W, wC);
		input.GetState(G_KEY_S, sC);
		input.GetState(G_KEY_A, aC);
		input.GetState(G_KEY_D, dC);

		if (input.GetMouseDelta(xDelta, yDelta) == GW::GReturn::REDUNDANT) //Prevent Camera drift by ignoring 
		{																   //redundant movement
			xDelta = 0.0f;
			yDelta = 0.0f;
		}

		auto now = std::chrono::steady_clock::now();  //Collects the change over time between ticks
		deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - lastUpdate).count() / 1000000.0f;
		lastUpdate = now;


		float totalPitch = 1.13446 * yDelta / height; //Variables to calculate mouse movement
		float totalYaw = 1.13446 * aRatio * xDelta / width;
		float perFrameSpeed = cSpeed * deltaTime;
		yChange = spaceC - shiftC;
		zChange = wC - sC;
		xChange = dC - aC;


		//PITCH/////
		GW::MATH::GMATRIXF pitch;
		proxyMat.RotationYawPitchRollF(0, totalPitch, 0, pitch);
		if (xDelta != 0){
			proxyMat.MultiplyMatrixF(view, pitch, view);
		}
		else{
			proxyMat.MultiplyMatrixF(view, zero, view);
		}

		//YAW/////
		GW::MATH::GMATRIXF yaw;
		proxyMat.RotationYawPitchRollF(totalYaw, 0, 0, yaw);
		if (yDelta != 0){
			proxyMat.MultiplyMatrixF(yaw, view, view);
		}
		else{	
			proxyMat.MultiplyMatrixF(zero, view, view);
		}

		//X and Z TRANSLATION
		GW::MATH::GVECTORF zxMove = { xChange * perFrameSpeed,0,zChange * perFrameSpeed };
		proxyMat.TranslateGlobalF(view, zxMove, view);

		//Y TRANSLATION/////
		GW::MATH::GVECTORF yMove = { 0,yChange * cSpeed * deltaTime ,0 };
		proxyMat.TranslateGlobalF(view, yMove, view);

		currView = view;

		proxyMat.InverseF(view, view);

	}

	void SwapLevel(GW::AUDIO::GAudio audio) //Unloads One level to load another
	{
		float lvlone;
		float lvltwo;
		input.GetState(G_KEY_1, lvlone);
		input.GetState(G_KEY_2, lvltwo);

		if (lvltwo != 0)
		{
			theLevel.UnloadLevel();
			theLevel.LoadLevel("../Levels/GameLevelTwo.txt", "../Models", log);
			theLevel.UploadLevelToGPU(d3d, world, view, pers);
			lvltwo = 0;
			audio.PlaySounds();
		}
		else if (lvlone != 0)
		{
			theLevel.UnloadLevel();
			theLevel.LoadLevel("../Levels/GameLevelOne.txt", "../Models", log);
			theLevel.UploadLevelToGPU(d3d, world, view, pers);
			lvlone = 0;
			audio.PlaySounds();
		}
		
		lvlone = 0;
		lvltwo = 0;

	}

	void CreateMatricies(GW::GRAPHICS::GDirectX11Surface _d3d)
	{
		//WORLD MATRIX//////////
		world = GW::MATH::GIdentityMatrixF;

		//VIEW MATRIX//////////
		view = GW::MATH::GIdentityMatrixF;
		GW::MATH::GVECTORF eye = { 0, 8, -18, 0 };
		GW::MATH::GVECTORF at = { 0, 0, 0, 0 };
		GW::MATH::GVECTORF up = { 0, 1, 0, 0 };
		proxyMat.LookAtLHF(eye, at, up, view);

		//PROJECTION MATRIX//////////
		float ratio = 0.0f;
		_d3d.GetAspectRatio(ratio);
		float fov = G_DEGREE_TO_RADIAN_F(65);
		float zFar = 0.1f;
		float zNear = 100.0f;
		proxyMat.ProjectionDirectXLHF(fov, ratio, zFar, zNear, pers);
		
	
	}

public:
	~RenderManager()
	{
		// ComPtr will auto release so nothing to do here yet 
	}



};
