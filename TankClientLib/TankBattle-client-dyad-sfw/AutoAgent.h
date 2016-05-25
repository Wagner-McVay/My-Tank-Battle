#pragma once

#include "Vector2.h"
#include <iostream>
#include "Graph.h"
#include "Solver.h"
#include "Grid.h"

#include "IAgent.h"
#include "sfwdraw.h"

/*
    Example dumb agent.

    State Description:
        

        Cannon:
            SCAN:
                Turns Right and Left covering a 162 degree ark
                enemy in sight? -> Set Target, AIM, CHASE
			AIM:
				Rotates cannon constantly to face target
				enemy in range? -> FIRE
				lost sight of enemy for 3 seconds? -> SCAN, WANDER
			FIRE:
                Fire weapon
                -> SCAN

        Tank:
            WANDER:
                Wanders Aimlessly
			CHASE:
				Moves tward Target
*/

class AutoAgent : public IAgent 
{
    // Cache current battle data on each update
    tankNet::TankBattleStateData current;

    // output data that we will return at end of update
    tankNet::TankBattleCommand tbc;

    // Could use this to keep track of the environment, check out the header.
    Grid map;

    //////////////////////
    // States for turret and tank
    enum TURRET { SCAN, AIM, FIRE } turretState = SCAN;
    enum TANK   { WANDER, CHASE   } tankState   = WANDER;

    float randTimer = 0;
	float spotTimer = 0;
	bool r = false;
	int t = 0;
    // Active target location to pursue
    Vector2 target;

    ///////
    // Scanning
	void scan()
	{
		Vector2 tf = Vector2::fromXZ(current.cannonForward);  // current forward
		Vector2 cp = Vector2::fromXZ(current.position); // current position
		Vector2 cf = Vector2::fromXZ(current.forward);  // current forward

		tbc.fireWish = false;

		// Arbitrarily look around for an enemy.
		//tbc.cannonMove = tankNet::CannonMovementOptions::RIGHT;

		// look left and right
		if (cross(cf, tf) >= 0.9f) { r = false; }
		if (cross(cf, tf) <= -0.9f) { r = true; }
		if (r == true) tbc.cannonMove = tankNet::CannonMovementOptions::RIGHT;
		else tbc.cannonMove = tankNet::CannonMovementOptions::LEFT;

        // Search through the possible enemy targets
        for (int aimTarget = 0; aimTarget < current.playerCount - 1; ++aimTarget)
            if (current.tacticoolData[aimTarget].inSight && current.canFire) // if in Sight and we can fire
            {
				t = aimTarget;
				target = Vector2::fromXZ(current.tacticoolData[aimTarget].lastKnownPosition);
				tankState = CHASE;
				if (dot(tf, normal(target - cp)) > .87f)
					turretState = AIM;
				break;
            }
    }

	void aim()
	{
		Vector2 tf = Vector2::fromXZ(current.cannonForward);  // current forward
		Vector2 cp = Vector2::fromXZ(current.position); // current position
		Vector2 cf = Vector2::fromXZ(current.forward);  // current forward
	
		tbc.fireWish = false;

		spotTimer += sfw::getDeltaTime();
		if (spotTimer >= 3 || cp == target)
		{
			tankState = WANDER;
			turretState = SCAN;
		}
		if (current.tacticoolData[t].inSight) // if in Sight
		{
			// reset give up time
			spotTimer = 0;
			// update last Known Position
			target = Vector2::fromXZ(current.tacticoolData[t].lastKnownPosition);
		}
		if (current.tacticoolData[t].inSight && distance(target, cp) < 30 )
		{
			turretState = FIRE;
		}
		if (cross(tf, target - cp) > 0) { tbc.cannonMove = tankNet::CannonMovementOptions::RIGHT; }
		else { tbc.cannonMove = tankNet::CannonMovementOptions::LEFT; }

	}

    void fire()
    {
        // No special logic for now, just shoot.
        tbc.fireWish = current.canFire;
		tankState = CHASE;
		turretState = AIM;
    }

    std::list<Vector2> path;
    void wander()
    {
        Vector2 cp = Vector2::fromXZ(current.position); // current position
        Vector2 cf = Vector2::fromXZ(current.forward);  // current forward

        
        randTimer -= sfw::getDeltaTime();

        // If we're pretty close to the target, get a new target           
        if (distance(target, cp) < 20 || randTimer < 0)
        {
            target = Vector2::random() * Vector2 { 50, 50 };
            randTimer = rand() % 4 + 2; // every 2-6 seconds randomly pick a new target
        }

        // determine the forward to the target (which is the next waypoint in the path)
        Vector2 tf = normal(target - cp);        

        if (dot(cf, tf) > .87f) // if the dot product is close to lining up, move forward
            tbc.tankMove = tankNet::TankMovementOptions::FWRD;
		else // otherwise turn right or left until we line up!
		{
			if (cross(cf, tf) >= 0)
			{tbc.tankMove = tankNet::TankMovementOptions::RIGHT;}
			else
			{tbc.tankMove = tankNet::TankMovementOptions::LEFT;}
		}
    }

	void chase()
	{
		Vector2 cp = Vector2::fromXZ(current.position); // current position
		Vector2 cf = Vector2::fromXZ(current.forward);  // current forward

		// determine the forward to the target (which is the next waypoint in the path)
		Vector2 tf = normal(target - cp);

		if (dot(cf, tf) > .87f) // if the dot product is close to lining up, move forward
		{	// If we're pretty close to the target, reverse          

			tbc.tankMove = tankNet::TankMovementOptions::FWRD;
		}
		else // otherwise turn right or left until we line up!
		{
			if (cross(cf, tf) >= 0)
			{
				tbc.tankMove = tankNet::TankMovementOptions::RIGHT;
			}
			else
			{
				tbc.tankMove = tankNet::TankMovementOptions::LEFT;
			}
		}
	}
	
public:
    tankNet::TankBattleCommand update(tankNet::TankBattleStateData *state)
    {
        current = *state;
        tbc.msg = tankNet::TankBattleMessage::GAME;

        switch (turretState)
        {
        case SCAN: scan(); break;
		case AIM: aim(); break;
        case FIRE: fire(); break;
        }

        switch (tankState)
        {
        case WANDER: wander(); break;
		case CHASE: chase(); break;
        }

        return tbc;
    }
};