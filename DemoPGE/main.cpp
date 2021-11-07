// O------------------------------------------------------------------------------O
// | Breakout Clone (main.cpp)                                                    |
// O------------------------------------------------------------------------------O

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <string>

// Override base class with your custom functionality
class BreakOut : public olc::PixelGameEngine
{
public:
	BreakOut()
	{
		// Name your application
		sAppName = "BreakOut Clone";
	}

private:
	enum class tType
	{
		EMPTY, BORDER, RED, GREEN, YELLOW, BAT
	};

	const int world_width = 24;
	const int world_height = 28;

	olc::vi2d vBlockSize = { 16,16 };

	olc::vf2d vBatSize = { 3.0f, 0.5f };
	olc::vf2d vBatPos = { (world_width / 2.0f) - vBatSize.x / 2.0f, float(world_height) - 4.0f };
	olc::vf2d fNextBatPos = vBatPos;

	olc::vf2d vBallPos = { 0.0f, 0.0f };
	olc::vf2d vBallDir = { 0.0f, 0.0f };
	float fBallSpeed = 15.0f;
	float fBallRadius = 5.0f;

	float fBatSpeed = 20.0f;

	
	std::unique_ptr<tType[]> blocks;
	std::unique_ptr<olc::Sprite> sprTile;

	std::unique_ptr<olc::Sprite> sprFragment;
	std::unique_ptr<olc::Decal> decFragment;

	const float MIN_ANGLE = 0.523;  // radian (30 degrees)
	const float MAX_ANGLE = 1.221;  // radian (70 degrees)

	int numBlocks = 0;

	struct sFragment
	{
		olc::vf2d pos;
		olc::vf2d vel;
		float fAngle;
		float fTime;
		olc::Pixel colour;
	};

	std::list<sFragment> listFragments;

	int high_score = 0;
	int score = 0;
	int lives = 3;

	enum class eState
	{
		NEW_GAME, ACTIVE, MISS, GAME_OVER, WIN
	};
	eState game_state = eState::NEW_GAME;


public:
	int getScreenWidth() {
		return (world_width+8) * vBlockSize.y;
	}

	int getScreenHeight() {
		return world_height * vBlockSize.x;
	}

	void pauseBall()
	{
		vBallDir = { 0.0f, 0.0f };
		vBallPos = { 28.0f, -10.0f };  // off the screen
	}

	void restartBall()
	{
		float fAngle = 0.0;

		// Select an angle > than MIN_ANGLE < MAX_ANGLE
		while (!((fAngle > MIN_ANGLE) && (fAngle < MAX_ANGLE)) )
		{
			fAngle = 2.0f * 3.14159f * float(rand()) / float(RAND_MAX);
		}

		vBallDir = { cos(fAngle), sin(fAngle) };
		vBallPos = { 12.5f, 15.5f };
	}

	void resetWorld()
	{
		score = 0;
		lives = 3;
		numBlocks = 0;

		for (int y = 0; y < world_height; y++)
		{
			for (int x = 0; x < world_width; x++)
			{
				// Draw the border blocks
				if (x == 0 || y == 0 || x == world_width - 1)
					blocks[y * world_width + x] = tType::BORDER;
				else
					blocks[y * world_width + x] = tType::EMPTY;

				// Two rows of red blocks
				if (x > 2 && x <= 20 && y > 3 && y <= 4) {
					blocks[y * world_width + x] = tType::RED;
					numBlocks++;
				}

				// Two rows of green blocks
				if (x > 2 && x <= 20 && y > 5 && y <= 6) {
					blocks[y * world_width + x] = tType::GREEN;
					numBlocks++;
				}

				// Two rows of yellow blocks
				if (x > 2 && x <= 20 && y > 7 && y <= 8) {
					blocks[y * world_width + x] = tType::YELLOW;
					numBlocks++;
				}
			}
		}
	}

	bool OnUserCreate() override
	{
		// Called once at the start, so create things here
		blocks = std::make_unique<tType[]>(world_width * world_height);
		resetWorld();

		// Load the sprite
		sprTile = std::make_unique<olc::Sprite>("./assets/gfx/tut_tiles.png");

		// Load Fragment Sprite
		sprFragment = std::make_unique<olc::Sprite>("./assets/gfx/tut_fragment.png");

		// Create decal of fragment
		decFragment = std::make_unique<olc::Decal>(sprFragment.get());

		// Start Ball
		//restartBall();

		game_state = eState::NEW_GAME;


		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{	
		if (game_state == eState::NEW_GAME || game_state == eState::MISS || 
				game_state == eState::GAME_OVER || game_state == eState::WIN) {
			if (GetKey(olc::Key::SPACE).bPressed || GetMouse(0).bPressed) {
				if (game_state == eState::GAME_OVER || game_state == eState::WIN) {
					game_state = eState::NEW_GAME;
					resetWorld();
				}
				else {
					game_state = eState::ACTIVE;
					restartBall();
				}
			}
		}

		// Handle User Input
		if (GetKey(olc::Key::LEFT).bHeld) fNextBatPos.x -= fBatSpeed * fElapsedTime;
		if (GetKey(olc::Key::RIGHT).bHeld) fNextBatPos.x += fBatSpeed * fElapsedTime;

		// Control the paddle (bat) with the scroll wheel
		fNextBatPos.x += GetMouseWheel() * (fBatSpeed/2) * fElapsedTime;

		if (fNextBatPos.x < 1.0f) {
			fNextBatPos.x = 1.0f;
		}
		if (fNextBatPos.x + vBatSize.x > world_width -1)
		{
			fNextBatPos.x = world_width - (vBatSize.x + 1);
		}

		// A better collision detection
		// Calculate where ball should be, if no collision
		olc::vf2d vPotentialBallPos = vBallPos + vBallDir * fBallSpeed * fElapsedTime;

		// Test for hits 4 points around ball
		olc::vf2d vTileBallRadialDims = { fBallRadius / vBlockSize.x, fBallRadius / vBlockSize.y };

		auto TestResolveCollisionPoint = [&](const olc::vf2d& point, olc::vf2d& hitpos, tType& id)
		{
			olc::vi2d vTestPoint = vPotentialBallPos + vTileBallRadialDims * point;

			auto& tile = blocks[vTestPoint.y * world_width + vTestPoint.x];
			if (tile == tType::EMPTY)
			{
				// Check if Bat hit
				bool bBatHit = (vTestPoint.y >= vBatPos.y) && (vTestPoint.x >= vBatPos.x) && (vTestPoint.x <= vBatPos.x + vBatSize.x);
				if (bBatHit)
				{
					vBallDir.y *= -1.0f;  // Bat hit, switch ball direction
					vBallPos.y = vBatPos.y - vTileBallRadialDims.y;
					score += 5;		// add to score when you hit the ball
				}

				// Do Nothing, Act as though no collision (no fragments)
				return false;
			}
			else
			{
				// Ball has collided with a tile
				bool bTileHit = tile == tType::RED || tile == tType::GREEN || tile == tType::YELLOW ;
				if (bTileHit)
				{
					id = tile;
					hitpos = { float(vTestPoint.x), float(vTestPoint.y) };

					// Update the tile to new color
					switch (tile) {
						case tType::YELLOW: tile = tType::GREEN; break;
						case tType::GREEN: tile = tType::RED; break;
						case tType::RED: tile = tType::EMPTY; break;
					}
				}

				// Collision response
				if (point.x == 0.0f) vBallDir.y *= -1.0f;
				if (point.y == 0.0f) vBallDir.x *= -1.0f;
				return bTileHit;
			}
		};

		bool bHasHitTile = false;
		olc::vf2d hitpos;
		tType hitid = tType::EMPTY;
		bHasHitTile |= TestResolveCollisionPoint(olc::vf2d(0, -1), hitpos, hitid);
		bHasHitTile |= TestResolveCollisionPoint(olc::vf2d(0, +1), hitpos, hitid);
		bHasHitTile |= TestResolveCollisionPoint(olc::vf2d(-1, 0), hitpos, hitid);
		bHasHitTile |= TestResolveCollisionPoint(olc::vf2d(+1, 0), hitpos ,hitid);

		if (bHasHitTile)
		{
			int num_fragments = 0;
			switch (hitid) {
			case tType::RED: 
				score += 30; 
				num_fragments = 30;  
				numBlocks--;	// block destroyed
				if (numBlocks == 0) {
					game_state = eState::WIN;
					// pause the ball
					pauseBall();
				}
				break;
			case tType::GREEN: 
				score += 20; 
				num_fragments = 20; 
				break;
			case tType::YELLOW: 
				score += 10; 
				num_fragments = 10;  
				break;
			}
			for (int i = 0; i < num_fragments; i++)
			{
				sFragment f;
				f.pos = { hitpos.x + 0.5f, hitpos.y + 0.5f };
				float fAngle = float(rand()) / float(RAND_MAX) * 2.0f * 3.14159f;
				float fVelocity = float(rand()) / float(RAND_MAX) * 10.0f;
				f.vel = { fVelocity * cos(fAngle), fVelocity * sin(fAngle) };
				f.fAngle = fAngle;
				f.fTime = 3.0f;
				if (hitid == tType::RED) f.colour = olc::RED;
				if (hitid == tType::GREEN) f.colour = olc::GREEN;
				if (hitid == tType::YELLOW) f.colour = olc::YELLOW;
				listFragments.push_back(f);
			}
		}

		// Fake Floor
		// XXX if (vBallPos.y > 20.0f) vBallDir.y *= -1.0f;


		// Check if ball has gone off screen
		if (vBallPos.y  > world_height - 2)
		{
			// pause the ball
			pauseBall();
			game_state = eState::MISS;
			lives -= 1;
			if (lives <= 0)
			{
				lives = 0;
				game_state = eState::GAME_OVER;
			}
		}

		// Actually update ball position with modified direction
		vBallPos += vBallDir * fBallSpeed * fElapsedTime;

		// Actually update the bat position
		vBatPos = fNextBatPos;

		// Update fragments
		for (auto& f : listFragments)
		{
			f.vel += olc::vf2d(0.0f, 20.0f) * fElapsedTime; // ??? fElapsedTime here ???
			f.pos += f.vel * fElapsedTime;
			f.fAngle + 5.0f * fElapsedTime;
			f.fTime -= fElapsedTime;
			f.colour.a = (f.fTime / 3.0f) * 255;
		}

		// Remove dead fragments
		listFragments.erase(
			std::remove_if(listFragments.begin(), listFragments.end(), [](const sFragment& f) {return f.fTime < 0.0f;  }),
			listFragments.end()
		);




		// Draw Screen
		Clear(olc::DARK_BLUE);
		SetPixelMode(olc::Pixel::MASK); // Don't draw pixels which have any transparency

		for (int y = 0; y < world_height; y++)
		{
			for (int x = 0; x < world_width; x++)
			{
				switch (blocks[y * world_width + x])
				{
				case tType::EMPTY : // Do nothing
					break;
				case tType::BORDER: // Draw Boundary
					DrawPartialSprite(olc::vi2d(x, y) * vBlockSize, sprTile.get(), olc::vi2d(0,0) * vBlockSize, vBlockSize);
					break;
				case tType::RED: // Draw Red Block
					DrawPartialSprite(olc::vi2d(x, y) * vBlockSize, sprTile.get(), olc::vi2d(1,0) * vBlockSize, vBlockSize);
					break;
				case tType::GREEN: // Draw Green Block
					DrawPartialSprite(olc::vi2d(x, y) * vBlockSize, sprTile.get(), olc::vi2d(2, 0) * vBlockSize, vBlockSize);
					break;
				case tType::YELLOW: // Draw Yellow Block
					DrawPartialSprite(olc::vi2d(x, y) * vBlockSize, sprTile.get(), olc::vi2d(3, 0) * vBlockSize, vBlockSize);
					break;
				}
			}
		}
		SetPixelMode(olc::Pixel::NORMAL);  //Draw all pixels

		// Draw Ball
		FillCircle(vBallPos * vBlockSize, fBallRadius, olc::CYAN);

		// Draw Bat
		FillRect(vBatPos* vBlockSize, vBatSize * vBlockSize, olc::GREEN);

		// Draw Fragements
		for (auto& f : listFragments)
		{
			DrawRotatedDecal(f.pos* vBlockSize, decFragment.get(), f.fAngle, { 4,4 }, { 1,1 }, f.colour);
		}

		// Draw Score Board
		std::string sText = "High Score";
		DrawStringProp(25 * vBlockSize.x, 2 * vBlockSize.y, sText);
		sText = std::to_string(high_score);
		DrawStringProp(28 * vBlockSize.x, 3 * vBlockSize.y, sText);

		sText = "Score";
		DrawStringProp(25 * vBlockSize.x, 5 * vBlockSize.y, sText);
		sText = std::to_string(score);
		DrawStringProp(28 * vBlockSize.x, 6 * vBlockSize.y, sText);

		sText = "Lives";
		DrawStringProp(25 * vBlockSize.x, 8 * vBlockSize.y, sText);
		sText = std::to_string(lives);
		DrawStringProp(28 * vBlockSize.x, 9 * vBlockSize.y, sText);




		if (game_state == eState::NEW_GAME)
		{
			// New Game
			std::string sText = "Press Space Bar to Start";
			DrawStringProp(7 * vBlockSize.x, 15 * vBlockSize.y, sText);
		}

		if (game_state == eState::MISS)
		{
			// They missed the ball!
			std::string sText = "Oops! Press Space Bar to Continue";
			DrawStringProp(5 * vBlockSize.x, 15 * vBlockSize.y, sText);
		}

		if (game_state == eState::GAME_OVER)
		{
			// The game is over
			std::string sText = "Game Over!";
			DrawStringProp(10 * vBlockSize.x, 15 * vBlockSize.y, sText);

			if (score >= high_score) {
				high_score = score;
				std::string sText = "Nice! New High Score!";
				DrawStringProp(8 * vBlockSize.x, 16 * vBlockSize.y, sText);
			}

			sText = "Press Space Bar to Play Again";
			DrawStringProp(6 * vBlockSize.x, 17 * vBlockSize.y, sText);
		}
		if (game_state == eState::WIN) {
			// You Win!
			// All blocks cleared
			std::string sText = "You Win!";
			DrawStringProp(10 * vBlockSize.x, 15 * vBlockSize.y, sText);

			if (score >= high_score) {
				high_score = score;
				std::string sText = "Nice! New High Score!";
				DrawStringProp(8 * vBlockSize.x, 16 * vBlockSize.y, sText);
			}

			sText = "Press Space Bar to Play Again";
			DrawStringProp(6 * vBlockSize.x, 17 * vBlockSize.y, sText);
		}


		return true;
	}
};

int main()
{
	BreakOut demo;
	if (demo.Construct(demo.getScreenWidth(), demo.getScreenHeight(), 2, 2))
		demo.Start();
	return 0;
}
