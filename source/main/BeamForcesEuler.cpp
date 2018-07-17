
#include "Beam.h"

#include "engine.h"
#include "Replay.h"
#include "FlexAirfoil.h"
#include "screwprop.h"
#include "buoyance.h"
#include "collisions.h"
#include "airbrake.h"
#include "BeamStats.h"
#include "CmdKeyInertia.h"

extern float mrtime;



void Beam::calcForcesEuler(int doUpdate, Real dt, int step, int maxstep, Beam** trucks, int numtrucks)
{
	// do not calc anything if we are going to get deleted
	if(deleting) return;

	int i,j;
	if (dt==0.0) return;
	if (reset_requested) return;

	int increased_accuracy=0;
	float inverted_dt=1.0f/dt;

	BES_START(BES_CORE_WholeTruckCalc);

	//engine callback
	if (state==ACTIVATED && engine)
	{
		BES_START(BES_CORE_TruckEngine);

		if(engine)
			engine->update(dt, doUpdate);

		BES_STOP(BES_CORE_TruckEngine);
	}
	//		if (doUpdate) mWindow->setDebugText(engine->status);


	BES_START(BES_CORE_Beams);

	//springs
	Vector3 dis;
	Vector3 v;
	for (i=0; i<free_beam; i++)
	{
		//trick for exploding stuff
		if (!beams[i].disabled)
		{
			//Calculate beam length
			if (!beams[i].p2truck) {
				dis=beams[i].p1->RelPosition;
				dis-=beams[i].p2->RelPosition;
			}
			else
			{
				dis=beams[i].p1->AbsPosition;
				dis-=beams[i].p2->AbsPosition;
			}

			Real dislen=dis.squaredLength();

			Real inverted_dislen=fast_invSqrt(dislen);
			dislen=dislen*inverted_dislen;

			//Calculate beam's deviation from normal
			Real difftoBeamL = dislen - beams[i].L;

			Real k=beams[i].k;
			Real d=beams[i].d;

			if (beams[i].bounded)
			{
				// We do a binary search in bounded's values to lessen the number of ifs
				if (beams[i].bounded<=SHOCK2)
				{
					// hard (normal) shock bump
					if (beams[i].bounded==SHOCK1)
					{
						if (difftoBeamL > beams[i].longbound*beams[i].L)
						{
							// Following code interpolates between defined beam parameters and default beam parameters
							float interp_ratio=difftoBeamL-beams[i].longbound*beams[i].L;

							// hard (normal) shock bump
							k=k+(DEFAULT_SPRING-k)*interp_ratio;
							d=d+(DEFAULT_DAMP-d)*interp_ratio;
						}
						else if (difftoBeamL < -beams[i].shortbound*beams[i].L)
						{
							// Following code interpolates between defined beam parameters and default beam parameters
							float interp_ratio=-beams[i].shortbound*beams[i].L-difftoBeamL;

							// hard (normal) shock bump
							k=k+(DEFAULT_SPRING-k)*interp_ratio;
							d=d+(DEFAULT_DAMP-d)*interp_ratio;
						}
					}
					else // We assume the bounded=SHOCK2 case
					{
 						calcShocks2(i, difftoBeamL, k, d);
					}
				}
				else
				{
					if (beams[i].bounded==ROPE)
					{
						if  (difftoBeamL<0.0f)
						{
							k=0.0f;
							d=d*0.1f;
						}
					}
					else // We assume bounded=SUPPORTBEAM
					{
						if (difftoBeamL>0.0f)
						{
							k=0.0f;
							d=d*0.1f;
							// If support beam is extended more than four beam lengths, break it
							if (difftoBeamL>beams[i].L*4.0f)
							{
								beams[i].broken=1;
								beams[i].disabled=true;
							}
						}
					}
				}
			}

			//Calculate beam's rate of change
			v=beams[i].p1->Velocity;
			v-=beams[i].p2->Velocity;

			float flen;
			flen = -k*(difftoBeamL)-d*v.dotProduct(dis)*inverted_dislen;
			float sflen=flen;
			beams[i].stress=flen;
			flen=fabs(flen);


			// Fast test for deformation
			if (flen > beams[i].minmaxposnegstress)
			{
				if ((beams[i].type==BEAM_NORMAL || beams[i].type==BEAM_INVISIBLE) && beams[i].bounded!=SHOCK1 && k!=0.0f)
				{
					// Actual deformation tests
					// For compression
					if (sflen>beams[i].maxposstress && difftoBeamL<0.0f)
					{

						increased_accuracy=1;
						Real yield_length=beams[i].maxposstress/k;
						Real deform=difftoBeamL+yield_length*(1.0f-beams[i].plastic_coef);
						Real Lold=beams[i].L;
						beams[i].L+=deform;
						if (beams[i].L < MIN_BEAM_LENGTH) beams[i].L=MIN_BEAM_LENGTH;
						sflen=sflen-(sflen-beams[i].maxposstress)*0.5f;
						flen=sflen;
						if (beams[i].L>0.0f && beams[i].L<Lold)
						{
							beams[i].maxposstress*=Lold/beams[i].L;
						}

						//For the compression case we do not remove any of the beam's
						//strength for structure stability reasons
						//beams[i].strength=beams[i].strength+deform*k*0.5f;

						//Sound effect
						//Sound volume depends on the energy lost due to deformation (which gets converted to sound (and thermal) energy)
						/*ssm->modulate(trucknum, SS_MOD_CREAK, deform*k*(difftoBeamL+deform*0.5f));
						ssm->trigOnce(trucknum, SS_TRIG_CREAK);*/

						beams[i].minmaxposnegstress=std::min(beams[i].maxposstress, -beams[i].maxnegstress);
						beams[i].minmaxposnegstress=std::min(beams[i].minmaxposnegstress, beams[i].strength);
						if(beambreakdebug)
						{
							LogManager::getSingleton().logMessage(" YYY Beam " + StringConverter::toString(i) + " just deformed with compression force " + StringConverter::toString(flen) + " / " + StringConverter::toString(beams[i].strength) + ". It was between nodes " + StringConverter::toString(beams[i].p1->id) + " and " + StringConverter::toString(beams[i].p2->id) + ".");
						}
					} else	// For expansion
					if (sflen<beams[i].maxnegstress && difftoBeamL>0.0f)
					{
						increased_accuracy=1;
						Real yield_length=beams[i].maxnegstress/k;
						Real deform=difftoBeamL+yield_length*(1.0f-beams[i].plastic_coef);
						Real Lold=beams[i].L;
						beams[i].L+=deform;
						sflen=sflen-(sflen-beams[i].maxnegstress)*0.5f;
						flen=-sflen;
						if (Lold>0.0f && beams[i].L>Lold)
						{
							beams[i].maxnegstress*=beams[i].L/Lold;
						}
						beams[i].strength=beams[i].strength-deform*k;

						//Sound effect
						//Sound volume depends on the energy lost due to deformation (which gets converted to sound (and thermal) energy)
						/*ssm->modulate(trucknum, SS_MOD_CREAK, deform*k*(difftoBeamL+deform*0.5f));
						ssm->trigOnce(trucknum, SS_TRIG_CREAK);*/

						beams[i].minmaxposnegstress=std::min(beams[i].maxposstress, -beams[i].maxnegstress);
						beams[i].minmaxposnegstress=std::min(beams[i].minmaxposnegstress, beams[i].strength);
						if(beambreakdebug)
						{
							LogManager::getSingleton().logMessage(" YYY Beam " + StringConverter::toString(i) + " just deformed with extension force " + StringConverter::toString(flen) + " / " + StringConverter::toString(beams[i].strength) + ". It was between nodes " + StringConverter::toString(beams[i].p1->id) + " and " + StringConverter::toString(beams[i].p2->id) + ".");
						}
					}
				}

				// Test if the beam should be breaked
				if (flen > beams[i].strength)
				{
					// Sound effect.
					// Sound volume depends on spring's stored energy
#ifdef USE_OPENAL
					if(ssm) ssm->modulate(trucknum, SS_MOD_BREAK, 0.5*k*difftoBeamL*difftoBeamL);
					if(ssm) ssm->trigOnce(trucknum, SS_TRIG_BREAK);
#endif //OPENAL
					increased_accuracy=1;

					//Break the beam only when it is not connected to a node
					//which is a part of a collision triangle and has 2 "live" beams or less
					//connected to it.
					if (!((beams[i].p1->contacter && nodeBeamConnections(beams[i].p1->pos)<3) || (beams[i].p2->contacter && nodeBeamConnections(beams[i].p2->pos)<3)))
					{
						beams[i].broken=1;
						beams[i].disabled=true;
						sflen=0.0f;
						beams[i].p1->isSkin=true;
						beams[i].p2->isSkin=true;

						if(beambreakdebug)
						{
							LogManager::getSingleton().logMessage(" XXX Beam " + StringConverter::toString(i) + " just broke with force " + StringConverter::toString(flen) + " / " + StringConverter::toString(beams[i].strength) + ". It was between nodes " + StringConverter::toString(beams[i].p1->id) + " and " + StringConverter::toString(beams[i].p2->id) + ".");
						}

					} else beams[i].strength=2.0f*beams[i].minmaxposnegstress;

					//something broke, check buoyant hull
					int mk;
					for (mk=0; mk<free_buoycab; mk++)
					{
						int tmpv=buoycabs[mk]*3;
						if (buoycabtypes[mk]==BUOY_DRAGONLY) continue;
						if ((beams[i].p1==&nodes[cabs[tmpv]] || beams[i].p1==&nodes[cabs[tmpv+1]] || beams[i].p1==&nodes[cabs[tmpv+2]])
							&&(beams[i].p2==&nodes[cabs[tmpv]] || beams[i].p2==&nodes[cabs[tmpv+1]] || beams[i].p2==&nodes[cabs[tmpv+2]]))
							buoyance->setsink(1);
					}
				}
			}

			// At last update the beam forces
			Vector3 f=dis;
			f*=(sflen*inverted_dislen);
			beams[i].p1->Forces+=f;
			beams[i].p2->Forces-=f;
		}
	}

	BES_STOP(BES_CORE_Beams);
	BES_START(BES_CORE_AnimatedProps);

	//animate props
	for (int propi=0; propi<free_prop; propi++)
	{
		int animnum=0;
		float rx = 0.0f;
		float ry = 0.0f;
		float rz = 0.0f;

		while (props[propi].animFlags[animnum])
		{
			if (props[propi].animFlags[animnum])
			{
				float cstate = 0.0f;
				int div = 0.0f;
				int flagstate = props[propi].animFlags[animnum];
				float animOpt1 =props[propi].animOpt1[animnum];
				float animOpt2 =props[propi].animOpt2[animnum];
				float animOpt3 =props[propi].animOpt3[animnum];

				calcAnimators(flagstate, cstate, div, dt, animOpt1, animOpt2, animOpt3);

				// key triggered animations
				if ((props[propi].animFlags[animnum] & ANIM_FLAG_EVENT) && props[propi].animKey[animnum] != -1)
				{
					if (INPUTENGINE.getEventValue(props[propi].animKey[animnum]))
					{
						// keystatelock is disabled then set cstate
						if (props[propi].animKeyState[animnum] == -1.0f)
						{
							cstate += INPUTENGINE.getEventValue(props[propi].animKey[animnum]);
						} else if (!props[propi].animKeyState[animnum])
						{
							// a key was pressed and a toggle was done already, so bypass
							//toggle now
							if (!props[propi].lastanimKS[animnum])
							{
								props[propi].lastanimKS[animnum] = 1.0f;
								// use animkey as bool to determine keypress / release state of inputengine
								props[propi].animKeyState[animnum] = 1.0f;
							}
							else
							{
								props[propi].lastanimKS[animnum] = 0.0f;
								// use animkey as bool to determine keypress / release state of inputengine
								props[propi].animKeyState[animnum] = 1.0f;
							}
						} else
						{
							// bypas mode, get the last set position and set it
							cstate +=props[propi].lastanimKS[animnum];
						}
					} else
					{
						// keyevent exists and keylock is enabled but the key isnt pressed right now = get lastanimkeystatus for cstate and reset keypressed bool animkey
						if (props[propi].animKeyState[animnum] != -1.0f)
						{
							cstate +=props[propi].lastanimKS[animnum];
							props[propi].animKeyState[animnum] = 0.0f;
						}
					}
				}

				//propanimation placed here to avoid interference with existing hydros(cstate) and permanent prop animation
				//truck steering
				if (props[propi].animFlags[animnum] & ANIM_FLAG_STEERING) cstate += hydrodirstate;
				//aileron
				if (props[propi].animFlags[animnum] & ANIM_FLAG_AILERONS) cstate += hydroaileronstate;
				//elevator
				if (props[propi].animFlags[animnum] & ANIM_FLAG_ELEVATORS) cstate += hydroelevatorstate;
				//rudder Liftec
				if (props[propi].animFlags[animnum] & ANIM_FLAG_ARUDDER) cstate += hydrorudderstate;
				//permanent
				if (props[propi].animFlags[animnum] & ANIM_FLAG_PERMANENT) cstate += 1.0f;

				cstate *= props[propi].animratio[animnum];

				// autoanimate noflip_bouncer
				if (props[propi].animOpt5[animnum]) cstate *= (props[propi].animOpt5[animnum]);

				//rotate prop
				if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_X || props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y || props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z)
				{
					float limiter = 0.0f;
					if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
					{
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_X)
						{
							props[propi].rot = props[propi].rot * (Quaternion(Degree(0), Vector3::UNIT_Z) * Quaternion(Degree(0), Vector3::UNIT_Y) * Quaternion(Degree(cstate), Vector3::UNIT_X));
							props[propi].rotaX += cstate;
							limiter = props[propi].rotaX;
						}
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y)
						{
							props[propi].rot = props[propi].rot * (Quaternion(Degree(0), Vector3::UNIT_Z) * Quaternion(Degree(cstate), Vector3::UNIT_Y) * Quaternion(Degree(0), Vector3::UNIT_X));
							props[propi].rotaY += cstate;
							limiter = props[propi].rotaY;
						}
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z)
						{
							props[propi].rot = props[propi].rot * (Quaternion(Degree(cstate), Vector3::UNIT_Z) * Quaternion(Degree(0), Vector3::UNIT_Y) * Quaternion(Degree(0), Vector3::UNIT_X));
							props[propi].rotaZ += cstate;
							limiter = props[propi].rotaZ;
						}

					} else
					{
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_X) rx += cstate;
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y) ry += cstate;
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z) rz += cstate;
					}

					bool limiterchanged = false;
					// check if a positive custom limit is set to evaluate/calc flip back
					if (props[propi].animOpt2[animnum] - props[propi].animOpt4[animnum])
					{
						if (limiter > props[propi].animOpt2[animnum])
						{
							if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
							{
								limiter = props[propi].animOpt2[animnum];				// stop at limit
								props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if bounce is set
								limiterchanged = true;
							} else
							{
								limiter = props[propi].animOpt1[animnum];				// flip to other side at limit
								limiterchanged = true;
							}
						}
					} else
					{																	// no custom limit set, use 360�
						while (limiter > 180.0f)
						{
							if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
							{
								limiter = 180.0f;										// stop at limit
								props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if bounce is set
								limiterchanged = true;
							} else
							{
								limiter -= 360.0f;										// flip to other side at limit
								limiterchanged = true;
							}
						}
					}

					// check if a negative custom limit is set to evaluate/calc flip back
					if (props[propi].animOpt1[animnum] - props[propi].animOpt4[animnum])
					{
						if (limiter < (props[propi].animOpt1[animnum]))
						{
							if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
							{
								limiter = props[propi].animOpt1[animnum];				// stop at limit
								props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if active
								limiterchanged = true;
							} else
							{
								limiter = props[propi].animOpt2[animnum];				// flip to other side at limit
								limiterchanged = true;
							}
						}
					} else																// no custom limit set, use 360�
					{
						while (limiter < -180.0f)
						{
							if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
							{
								limiter = -180.0f;										// stop at limit
								props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if active
								limiterchanged = true;
							} else
							{
								limiter += 360.0f;										// flip to other side at limit including overflow
								limiterchanged = true;
							}
						}
					}

					if (limiterchanged)
					{
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_X) props[propi].rotaX = limiter;
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y) props[propi].rotaY = limiter;
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z) props[propi].rotaZ = limiter;
					}
				}

				//offset prop

				// TODO Unused Varaible
				//float ox = props[propi].orgoffsetX;

				// TODO Unused Varaible
				//float oy = props[propi].orgoffsetY;

				// TODO Unused Varaible
				//float oz = props[propi].orgoffsetZ;

				if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_X || props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Y || props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Z)
				{
					float offset = 0.0f;
					float autooffset = 0.0f;

					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_X) offset = props[propi].orgoffsetX;
					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Y) offset = props[propi].orgoffsetY;
					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Z) offset = props[propi].orgoffsetZ;

					offset += cstate;
					if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
					{
						autooffset = offset;
																							// check if a positive custom limit is set to evaluate/calc flip back
						if (props[propi].animOpt2[animnum] - props[propi].animOpt4[animnum])
						{
							if (autooffset > props[propi].animOpt2[animnum])
							{
								if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
								{
									autooffset = props[propi].animOpt2[animnum];			// stop at limit
									props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if active
								} else
									autooffset = props[propi].animOpt1[animnum];			// flip to other side at limit
							}
						} else																// no custom limit set, use 10x as default
							while (autooffset > 10.0f)
							{
								if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
								{
									autooffset = 10.0f;										// stop at limit
									props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if bounce is set
								} else
									autooffset -= 20.0f;									// flip to other side at limit including overflow
							}

																							// check if a negative custom limit is set to evaluate/calc flip back
						if (props[propi].animOpt1[animnum] - props[propi].animOpt4[animnum])
						{
							if (autooffset < (props[propi].animOpt1[animnum]))
							{
								if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
								{
									autooffset = props[propi].animOpt1[animnum];			// stop at limit
									props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if active
								} else
									autooffset = props[propi].animOpt2[animnum];			// flip to other side at limit
							}
						} else																// no custom limit set, use -10x�
							while (autooffset < -10.0f)
							{
								if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
								{
									autooffset = -10.0f;									// stop at limit
									props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if bounce is set
								} else
									autooffset += 20.0f;									// flip to other side at limit including overflow
							}
					}

					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_X)
					{
						props[propi].offsetx = offset;
						if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
							props[propi].orgoffsetX = autooffset;
					}
					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Y)
					{
						props[propi].offsety = offset;
						if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
							props[propi].orgoffsetY = autooffset;
					}
					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Z)
					{
						props[propi].offsetz = offset;
						if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
							props[propi].orgoffsetZ = autooffset;
					}
				}

			}
		animnum++;
		}
	//recalc the quaternions with final stacked rotation values ( rx, ry, rz )
	rx += props[propi].rotaX;
	ry += props[propi].rotaY;
	rz += props[propi].rotaZ;
	props[propi].rot = Quaternion(Degree(rz), Vector3::UNIT_Z) * Quaternion(Degree(ry), Vector3::UNIT_Y) * Quaternion(Degree(rx), Vector3::UNIT_X);
	}

	BES_STOP(BES_CORE_AnimatedProps);
	BES_START(BES_CORE_SkeletonColouring);

	//skeleton colouring
	if (((skeleton && doUpdate) || replay) )
	{
		for (i=0; i<free_beam; i++)
		{
			if (!beams[i].disabled)
			{
				if (((doUpdate && skeleton == 2) || replay) && !beams[i].broken && beams[i].mEntity && beams[i].mSceneNode)
				{
					float tmp=beams[i].stress/beams[i].minmaxposnegstress;
					float sqtmp=tmp*tmp;
					beams[i].scale = (sqtmp*sqtmp)*100.0f*sign(tmp);
				}
				if (doUpdate && skeleton == 1 && !beams[i].broken && beams[i].mEntity && beams[i].mSceneNode)
				{
					int scale=(int)beams[i].scale * 100;
					if(scale>100) scale=100;
					if(scale<-100) scale=-100;
					char bname[255];
					sprintf(bname, "mat-beam-%d", scale);
					beams[i].mEntity->setMaterialName(bname);
				}
				else if(doUpdate && skeleton && beams[i].mSceneNode && (beams[i].broken || beams[i].disabled) && beams[i].mSceneNode)
				{
					beams[i].mSceneNode->detachAllObjects();
				}
			}
		}
	}

	BES_STOP(BES_CORE_SkeletonColouring);
	BES_START(BES_CORE_Rigidifiers);

	//the rigidifiers
	for (i=0; i<free_rigidifier; i++)
	{
		//failsafe
		if ((rigidifiers[i].beama && rigidifiers[i].beama->broken) || (rigidifiers[i].beamc && rigidifiers[i].beamc->broken)) continue;
		//thanks to Chris Ritchey for the solution!
		Vector3 vab=rigidifiers[i].a->RelPosition-rigidifiers[i].b->RelPosition;
		Vector3 vcb=rigidifiers[i].c->RelPosition-rigidifiers[i].b->RelPosition;
		float vablen = vab.squaredLength();
		float vcblen = vcb.squaredLength();
		if (vablen == 0.0f || vcblen == 0.0f) continue;
		float inverted_vablen = fast_invSqrt(vablen);
		float inverted_vcblen = fast_invSqrt(vcblen);
		vablen = vablen*inverted_vablen;
		vcblen = vcblen*inverted_vcblen;
		vab = vab*inverted_vablen;
		vcb = vcb*inverted_vcblen;
		float vabdotvcb = vab.dotProduct(vcb);
		float alphap = vabdotvcb;
		if (alphap > 1.0f) alphap = 1.0f;
		else if (alphap<-1.0f) alphap = -1.0f;
		alphap=acos(alphap);
		float forcediv = -rigidifiers[i].k * (rigidifiers[i].alpha - alphap) + rigidifiers[i].d * (alphap - rigidifiers[i].lastalpha) * inverted_dt; //force dividend
		//forces at a
		float tmp = forcediv*inverted_vablen;
		Vector3 va = vcb * tmp - vab * (vabdotvcb * tmp);
		rigidifiers[i].a->Forces += va;
		//forces at c
		tmp = forcediv*inverted_vcblen;
		Vector3 vc = vab * tmp - vcb * (vabdotvcb * tmp);
		rigidifiers[i].c->Forces += vc;
		//reaction at b
		rigidifiers[i].b->Forces += -va - vc;
		rigidifiers[i].lastalpha=alphap;
	}


	BES_STOP(BES_CORE_Rigidifiers);
	BES_START(BES_CORE_Hooks);

	//aposition=Vector3::ZERO;
	if (state==ACTIVATED) //force feedback sensors
	{
		if (doUpdate)
		{
			affforce=nodes[cameranodepos[currentcamera]].Forces;
			affhydro=0;
		}
		else
		{
			affforce+=nodes[cameranodepos[currentcamera]].Forces;
		}
		for (int i=0; i<free_hydro; i++)
		{
			if (beams[hydro[i]].hydroFlags&(HYDRO_FLAG_DIR|HYDRO_FLAG_SPEED))
				affhydro+=beams[hydro[i]].hydroRatio*beams[hydro[i]].refL*beams[hydro[i]].stress;
		}
	}


	//locks - this is not active in network mode
	for(std::vector<hook_t>::iterator it=hooks.begin(); it!=hooks.end(); it++)
	{
		if (it->lockNode && it->locked == LOCKED)
		{
			it->lockNode->lockedPosition = it->hookNode->AbsPosition;
			it->lockNode->lockedVelocity = it->hookNode->Velocity;
			it->hookNode->Forces         = it->hookNode->Forces + it->lockNode->lockedForces;
		}
		if (it->lockNode && it->locked==PRELOCK)
		{
			//check for locking
			if ((it->hookNode->AbsPosition - it->lockNode->AbsPosition).squaredLength() < 0.00001f)
			{
				it->lockNode->lockednode=1;
				it->lockNode->lockedPosition = it->lockNode->AbsPosition;
				it->lockNode->lockedVelocity = it->lockNode->Velocity;
				it->lockNode->lockedForces   = it->lockNode->Forces;
				it->locked = LOCKED;
			}
			else
			{
				//add attraction forces
				Vector3 f = it->hookNode->AbsPosition - it->lockNode->AbsPosition;
				f.normalise();
				it->hookNode->Forces -= 100000.0 * f;
				it->lockNode->Forces += 100000.0 * f;
			}
		}
	}

	BES_STOP(BES_CORE_Hooks);
	BES_START(BES_CORE_Ropes);

	if (ropes.size())
	{
		for(std::vector <rope_t>::iterator it = ropes.begin(); it!=ropes.end(); it++)
		{
			if (it->lockedto)
			{
				it->beam->p2->AbsPosition = it->lockedto->AbsPosition;
				it->beam->p2->RelPosition = it->lockedto->AbsPosition - origin; //ropes[i].lockedtruck->origin; //we have a problem here
				it->beam->p2->Velocity = it->lockedto->Velocity;
				it->lockedto->Forces = it->lockedto->Forces + it->beam->p2->Forces;
				it->beam->p2->Forces=Vector3(0,0,0);
			}
		}
	}

	BES_STOP(BES_CORE_Ropes);

	//mouse stuff
	if (mousenode!=-1)
	{
		Vector3 dir=mousepos-nodes[mousenode].AbsPosition;
		nodes[mousenode].Forces += mousemoveforce * dir;
	}


	// START Slidenode section /////////////////////////////////////////////////
	// these must be done before the integrator, or else the forces are not calculated properly
	BES_START(BES_CORE_SlideNodes);
	updateSlideNodeForces(dt);
	BES_STOP(BES_CORE_SlideNodes);
	// END Slidenode section   /////////////////////////////////////////////////


	BES_START(BES_CORE_Nodes);

	float tminx=nodes[0].AbsPosition.x;
	float tmaxx=tminx;
	float tminy=nodes[0].AbsPosition.y;
	float tmaxy=tminy;
	float tminz=nodes[0].AbsPosition.z;
	float tmaxz=tminz;
	for (i=0; i<free_node; i++)
	{
		//if (_isnan(nodes[i].Position.length())) LogManager::getSingleton().logMessage("Node is NaN "+StringConverter::toString(i));

		//wetness
		if (nodes[i].wetstate==DRIPPING && !nodes[i].contactless)
		{
			nodes[i].wettime+=dt;
			if (nodes[i].wettime>5.0) nodes[i].wetstate=DRY; //dry!
			if (!nodes[i].iswheel && dripp) dripp->allocDrip(nodes[i].smoothpos, nodes[i].Velocity, nodes[i].wettime);
			//also for hot engine
			if (nodes[i].isHot && dustp) dustp->allocVapour(nodes[i].smoothpos, nodes[i].Velocity, nodes[i].wettime);
		}
		//locked nodes
		if (nodes[i].lockednode)
		{
			nodes[i].AbsPosition=nodes[i].lockedPosition;
			nodes[i].RelPosition=nodes[i].lockedPosition-origin;
			nodes[i].Velocity=nodes[i].lockedVelocity;
			nodes[i].lockedForces=nodes[i].Forces;
			nodes[i].Forces=Vector3(0,0,0);
		}

		//COLLISION
		if (!nodes[i].contactless)
		{
			nodes[i].colltesttimer+=dt;
			if (nodes[i].contacted || nodes[i].colltesttimer>0.005 || (nodes[i].iswheel && nodes[i].colltesttimer>0.0025) || increased_accuracy )
			{
				float ns=0;
				ground_model_t *gm = 0; // this is used as result storage, so we can use it later on
				int contacted = collisions->groundCollision(&nodes[i], nodes[i].colltesttimer, &gm, &ns);

				int handlernum = -1;
				//Tlooks like this needs to remain a binary operator otherwise the terrain material appears to be wrong
				if ( contacted |
					collisions->nodeCollision(&nodes[i], i==cinecameranodepos[currentcamera], contacted, nodes[i].colltesttimer, &ns, &gm, &handlernum))
				{
					//FX
					if (gm && doUpdate && dustp)
					{
						if (gm->fx_type==FX_DUSTY)
						{
							if(dustp) dustp->alloc(nodes[i].AbsPosition, nodes[i].Velocity/2.0, gm->fx_colour);
						}else if (gm->fx_type==FX_HARD)
						{
							float thresold=10.0;
							//smokey
							if (nodes[i].iswheel && ns>thresold)
							{
								if(dustp) dustp->allocSmoke(nodes[i].AbsPosition, nodes[i].Velocity);
#ifdef USE_OPENAL
								ssm->modulate(trucknum, SS_MOD_SCREETCH, (ns-thresold)/thresold);
								ssm->trigOnce(trucknum, SS_TRIG_SCREETCH);
#endif // USE_OPENAL
							}

							//sparks
							if (!nodes[i].iswheel && ns>1.0 && !nodes[i].disable_particles)
							{
								// friction < 10 will remove the 'f' nodes from the spark generation nodes
								if(sparksp) sparksp->allocSparks(nodes[i].AbsPosition, nodes[i].Velocity);
							}
						} else if (gm->fx_type==FX_CLUMPY)
						{
							if (clumpp && nodes[i].Velocity.squaredLength()>1.0) clumpp->allocClump(nodes[i].AbsPosition, nodes[i].Velocity/2.0, gm->fx_colour);
						}
					}

					// register wheel contact
					if (gm && useSkidmarks && nodes[i].wheelid >= 0)
					{
						if (!(nodes[i].iswheel%2))
							wheels[nodes[i].wheelid].lastContactInner = nodes[i].AbsPosition;
						else
							wheels[nodes[i].wheelid].lastContactOuter = nodes[i].AbsPosition;
						wheels[nodes[i].wheelid].lastContactType = (nodes[i].iswheel%2);
						wheels[nodes[i].wheelid].lastSlip = ns;
						wheels[nodes[i].wheelid].lastGroundModel = gm;
					}

					wheels[nodes[i].wheelid].lastEventHandler = handlernum;

					// note last ground model
					lastFuzzyGroundModel = gm;
				}
				nodes[i].colltesttimer=0.0;
			}
		}


		// record g forces on cameras
		if (i==cameranodepos[0])
		{
			cameranodeacc+=nodes[i].Forces*nodes[i].inverted_mass;
			cameranodecount++;
		}

		//integration
		if (!nodes[i].locked)
		{
			nodes[i].Velocity+=nodes[i].Forces*(dt*nodes[i].inverted_mass);
			nodes[i].RelPosition+=nodes[i].Velocity*dt;
			nodes[i].AbsPosition=origin;
			nodes[i].AbsPosition+=nodes[i].RelPosition;
		}
		if (nodes[i].AbsPosition.x>tmaxx) tmaxx=nodes[i].AbsPosition.x;
		else if (nodes[i].AbsPosition.x<tminx) tminx=nodes[i].AbsPosition.x;
		if (nodes[i].AbsPosition.y>tmaxy) tmaxy=nodes[i].AbsPosition.y;
		else if (nodes[i].AbsPosition.y<tminy) tminy=nodes[i].AbsPosition.y;
		if (nodes[i].AbsPosition.z>tmaxz) tmaxz=nodes[i].AbsPosition.z;
		else if (nodes[i].AbsPosition.z<tminz) tminz=nodes[i].AbsPosition.z;

		//prepare next loop (optimisation)
		//we start forces from zero
		//start with gravity
		nodes[i].Forces=nodes[i].gravimass;

		if (fuseAirfoil)
		{
			//aerodynamics on steroids!
			nodes[i].Forces+=fusedrag;
		}
		else
		{
			if (!disableDrag)
			{
				//add viscous drag (turbulent model)
				if (step&7 && !increased_accuracy)
				{
					//fasttrack drag
					nodes[i].Forces+=nodes[i].lastdrag;

				} else
				{
					Real speed=nodes[i].Velocity.squaredLength();//we will (not) reuse this
					speed=approx_sqrt(speed);
					//plus: turbulences
					Real defdragxspeed= DEFAULT_DRAG*speed;
					//Real maxtur=defdragxspeed*speed*0.01f;
					nodes[i].lastdrag=-defdragxspeed*nodes[i].Velocity;
					Real maxtur=defdragxspeed*speed*0.005f;
					nodes[i].lastdrag+=maxtur*Vector3(frand_11(), frand_11(), frand_11());
					nodes[i].Forces+=nodes[i].lastdrag;
				}
			}
		}
		//if in water
		watercontact=0;
		if (water)
		{
			//basic buoyance
			if (free_buoycab==0)
			{
				if (nodes[i].AbsPosition.y<water->getHeightWaves(nodes[i].AbsPosition))
				{
					watercontact=1;
					//water drag (turbulent)
					float velocityLength=nodes[i].Velocity.length();
					nodes[i].Forces-=(DEFAULT_WATERDRAG*velocityLength)*nodes[i].Velocity;
					nodes[i].Forces+=nodes[i].buoyancy*Vector3::UNIT_Y;
					//basic splashing
					if (splashp && water->getHeight()-nodes[i].AbsPosition.y<0.2 && nodes[i].Velocity.squaredLength()>4.0)
					{
						splashp->allocSplash(nodes[i].AbsPosition, nodes[i].Velocity);
						ripplep->allocRipple(nodes[i].AbsPosition, nodes[i].Velocity);
					}
					//engine stall
					if (i==cinecameranodepos[0] && engine) engine->stop();
					//wetness
					nodes[i].wetstate=WET;
				}
				else
				{
					if (nodes[i].wetstate==WET)
					{
						nodes[i].wetstate=DRIPPING;
						nodes[i].wettime=0;
					}
				}
			}
			else
			{
				if (nodes[i].AbsPosition.y<water->getHeightWaves(nodes[i].AbsPosition))
				{
					watercontact=1;
					//engine stall
					if (i==cinecameranodepos[0] && engine) engine->stop();
					//wetness
					nodes[i].wetstate=WET;
				}
				else
				{
					if (nodes[i].wetstate==WET)
					{
						nodes[i].wetstate=DRIPPING;
						nodes[i].wettime=0;
					}
				}
			}
		}
	}

	// anti-explosion guard
	// rationale behind 1e9 number:
	// - while 1e6 is reachable by a fast vehicle, it will be badly deformed and shaking due to loss of precision in calculations
	// - at 1e7 any typical RoR vehicle falls apart and stops functioning
	// - 1e9 may be reachable only by a vehicle that is 1000 times bigger than a typical RoR vehicle, and it will be a loooong trip
	// to be able to travel such long distances will require switching physics calculations to higher precision numbers
	// or taking a different approach to the simulation (truck-local coordinate system?)
	if (!inRange(tminx+tmaxx+tminy+tmaxy+tminz+tmaxz, -1e9, 1e9))
	{
		reset_requested=1; // truck exploded, schedule reset
		return; // return early to avoid propagating invalid values
	}

	minx=tminx-0.3;maxx=tmaxx+0.3;
	miny=tminy-0.3;maxy=tmaxy+0.3;
	minz=tminz-0.3;maxz=tmaxz+0.3;

	BES_STOP(BES_CORE_Nodes);
	BES_START(BES_CORE_Turboprop);

	//turboprop forces
	for (i=0; i<free_aeroengine; i++)
		if(aeroengines[i]) aeroengines[i]->updateForces(dt, doUpdate);

	BES_STOP(BES_CORE_Turboprop);
	BES_START(BES_CORE_Screwprop);

	//screwprop forces
	for (i=0; i<free_screwprop; i++)
		if(screwprops[i]) screwprops[i]->updateForces(doUpdate);

	BES_STOP(BES_CORE_Screwprop);
	BES_START(BES_CORE_Wing);

	//wing forces
	for (i=0; i<free_wing; i++)
		if(wings[i].fa) wings[i].fa->updateForces();

	BES_STOP(BES_CORE_Wing);
	BES_START(BES_CORE_FuseDrag);

	//compute fuse drag
	if (fuseAirfoil)
	{
		Vector3 wind=-fuseFront->Velocity;
		float wspeed=wind.length();
		Vector3 axis=fuseFront->RelPosition-fuseBack->RelPosition;
		float s=axis.length()*fuseWidth;
		float cz, cx, cm;
		float v=axis.getRotationTo(wind).w;
		float aoa=0;
		if (v<1.0 && v>-1.0) aoa=2.0*acos(v); //quaternion fun
		fuseAirfoil->getparams(aoa, 1.0, 0.0, &cz, &cx, &cm);

		//tropospheric model valid up to 11.000m (33.000ft)
		float altitude=fuseFront->AbsPosition.y;

		// TODO Unused Varaible
		//float sea_level_temperature=273.15f+15.0f; //in Kelvin
		float sea_level_pressure=101325; //in Pa

		// TODO Unused Varaible
		//float airtemperature=sea_level_temperature-altitude*0.0065f; //in Kelvin
		float airpressure=sea_level_pressure*approx_pow(1.0-0.0065*altitude/288.1, 5.24947); //in Pa
		float airdensity=airpressure*0.0000120896f;//1.225 at sea level

		//fuselage as an airfoil + parasitic drag (half fuselage front surface almost as a flat plane!)
		fusedrag=((cx*s+fuseWidth*fuseWidth*0.5)*0.5*airdensity*wspeed/free_node)*wind; //free_node is never null
	}

	BES_STOP(BES_CORE_FuseDrag);
	BES_START(BES_CORE_Airbrakes);

	//airbrakes
	for (int i=0; i<free_airbrake; i++)
	{
		airbrakes[i]->applyForce();
	}

	BES_STOP(BES_CORE_Airbrakes);
	BES_START(BES_CORE_Buoyance);

	//water buoyance
	mrtime+=dt;
	if (free_buoycab && water)
	{
		if (!(step%20))
		{
			//clear forces
			for (i=0; i<free_buoycab; i++)
			{
				int tmpv=buoycabs[i]*3;
				nodes[cabs[tmpv]].buoyanceForce=0;
				nodes[cabs[tmpv+1]].buoyanceForce=0;
				nodes[cabs[tmpv+2]].buoyanceForce=0;
			}
			//add forces
			for (i=0; i<free_buoycab; i++)
			{
				int tmpv=buoycabs[i]*3;
				buoyance->computeNodeForce(&nodes[cabs[tmpv]], &nodes[cabs[tmpv+1]], &nodes[cabs[tmpv+2]], doUpdate, buoycabtypes[i]);
			}
		}
		//apply forces
		for (i=0; i<free_node; i++)
		{
			nodes[i].Forces+=nodes[i].buoyanceForce;
		}

	}

	BES_STOP(BES_CORE_Buoyance);
	BES_START(BES_CORE_Axles);

	//wheel speed
	Real wspeed=0;
	//wheel stuff

	float engine_torque=0.0;

	// calculate torque per wheel
	if (state == ACTIVATED && engine && proped_wheels != 0)
		engine_torque = engine->getTorque()/proped_wheels;

	int propcounter=0;
	float torques[MAX_WHEELS]; // not used
	float newspeeds[MAX_WHEELS];

	float intertorque[MAX_WHEELS] = {0.0f}; //bad initialization
	//old-style viscous code
	if( free_axle == 0)
	{
		//first, evaluate torque from inter-differential locking
		for (i=0; i<proped_wheels/2-1; i++)
		{
			float speed1=(wheels[proppairs[i*2]].speed+wheels[proppairs[i*2+1]].speed)*0.5f;
			float speed2=(wheels[proppairs[i*2+2]].speed+wheels[proppairs[i*2+3]].speed)*0.5f;
			float torque=(speed1-speed2)*10000.0f;
			intertorque[i*2]-=torque*0.5f;
			intertorque[i*2+1]-=torque*0.5f;
			intertorque[i*2+2]+=torque*0.5f;
			intertorque[i*2+3]+=torque*0.5f;
		}
	}


	// new-style Axles
	// loop through all axles for interaxle torque, this is the torsion to keep
	// the axles aligned with each other as if they connected by a shaft
	for (i = 1; i < free_axle; ++i)
	{
		Ogre::Real axle_torques[2] = {0.0f, 0.0f};
		differential_data_t diff_data =
		{
			{
				(wheels[axles[i-1].wheel_1].speed + wheels[axles[i-1].wheel_2].speed) * 0.5f,
				(wheels[axles[i].wheel_1].speed + wheels[axles[i].wheel_2].speed) * 0.5f
			},
			axles[i-1].delta_rotation,
			{ axle_torques[0], axle_torques[1] },
			0, // no input torque, just calculate forces from different axle positions
			dt
		};

#if 0
		// use an open diff just for fun :)
		Axle::calcOpenDiff( diff_data );
#else
		// use the locked diff, most vehicles are setup this way...
		Axle::calcLockedDiff( diff_data );
#endif

		axles[i-1].delta_rotation = diff_data.delta_rotation;
		axles[i].delta_rotation = -diff_data.delta_rotation;

		intertorque[axles[i-1].wheel_1] = diff_data.out_torque[0];
		intertorque[axles[i-1].wheel_2] = diff_data.out_torque[0];
		intertorque[axles[i].wheel_1] = diff_data.out_torque[1];
		intertorque[axles[i].wheel_2] = diff_data.out_torque[1];
	}

	// loop through all the wheels (new style again)
	for (i = 0; i < free_axle; ++i)
	{
		Ogre::Real axle_torques[2] = {0.0f, 0.0f};
		wheel_t *axle_wheels[2] = { &wheels[axles[i].wheel_1], &wheels[axles[i].wheel_2] };

		differential_data_t diff_data =
		{
			{ axle_wheels[0]->speed, axle_wheels[1]->speed },
			axle_wheels[0]->delta_rotation,
			{ axle_torques[0], axle_torques[1] },
			// twice the torque since this is for two wheels, plus extra torque from
			// inter-axle torsion
			2.0f * engine_torque + intertorque[axles[i].wheel_1],
			dt
		};

		axles[i].calcTorque( diff_data );

		axle_wheels[0]->delta_rotation = diff_data.delta_rotation;
		axle_wheels[1]->delta_rotation = -diff_data.delta_rotation;

		intertorque[axles[i].wheel_1] = diff_data.out_torque[0];
		intertorque[axles[i].wheel_2] = diff_data.out_torque[1];
	}
	BES_STOP(BES_CORE_Axles);
	BES_START(BES_CORE_Wheels);

	for (i=0; i<free_wheel; i++)
	{
		Real speedacc=0.0;

		//total torque estimation
		Real total_torque = 0.0;
		if (wheels[i].propulsed > 0)
			total_torque=((free_axle == 0) ? engine_torque : intertorque[i]);

		//braking
		//ignore all braking code if the current wheel is not braked...
		if (wheels[i].braked)
		{
			//handbrake
			float hbrake = 0.0f;
			if (parkingbrake && wheels[i].braked != 4)
				hbrake = hbrakeforce;
			//directional braking
			float dbrake=0.0;
			if (wheels[i].braked==2 && hydrodirstate>0.0 && WheelSpeed<20.0) dbrake=brakeforce*hydrodirstate;
			if (wheels[i].braked==3 && hydrodirstate<0.0 && WheelSpeed<20.0) dbrake=brakeforce*-hydrodirstate;

			if ((brake != 0.0 || dbrake != 0.0 || hbrake != 0.0) && braked_wheels != 0)
			{
				if( fabs(wheels[i].speed) > 0.1f )
					total_torque -= (wheels[i].speed/fabs(wheels[i].speed))*(brake + dbrake + hbrake);
				// wheels are stopped, really this should
				else if( fabs(wheels[i].speed) > 0.0f)
					total_torque -= (wheels[i].speed/fabs(wheels[i].speed))*(brake + dbrake + hbrake)*1.2;
			}
		}
		//friction
		total_torque -= wheels[i].speed*1.0; //it is important to keep some wheel friction to avoid numerical instabilities

		// old-style
		if ( free_axle == 0 && wheels[i].propulsed > 0)
		{
			//differential locking
			if (i%2)
				total_torque-=(wheels[i].speed-wheels[i-1].speed)*10000.0;
			else
				total_torque-=(wheels[i].speed-wheels[i+1].speed)*10000.0;
			//inter differential locking
			total_torque+=intertorque[propcounter];
			propcounter++;
		}

		//application to wheel
		torques[i]=total_torque;
		Vector3 axis=wheels[i].refnode1->RelPosition-wheels[i].refnode0->RelPosition;
		float axis_precalc=total_torque/(Real)(wheels[i].nbnodes);
		axis=fast_normalise(axis);

		for (j=0; j<wheels[i].nbnodes; j++)
		{
			Vector3 radius;
			if (j%2)
				radius=wheels[i].nodes[j]->RelPosition-wheels[i].refnode1->RelPosition;
			else
				radius=wheels[i].nodes[j]->RelPosition-wheels[i].refnode0->RelPosition;
				float inverted_rlen=fast_invSqrt(radius.squaredLength());

			if (wheels[i].propulsed==2)
				radius=-radius;

			Vector3 dir=axis.crossProduct(radius);
			wheels[i].nodes[j]->Forces+=dir*(axis_precalc*inverted_rlen*inverted_rlen);
			//wheel speed
			if (j%2) speedacc+=(wheels[i].nodes[j]->Velocity-wheels[i].refnode1->Velocity).dotProduct(dir)*inverted_rlen;
			else speedacc+=(wheels[i].nodes[j]->Velocity-wheels[i].refnode0->Velocity).dotProduct(dir)*inverted_rlen;
		}
		//wheel speed
		newspeeds[i]=speedacc/wheels[i].nbnodes;
		if (wheels[i].propulsed==1)
			wspeed+=newspeeds[i];
		//for network
		wheels[i].rp+=(newspeeds[i]/wheels[i].radius)*dt;
		//reaction torque
		Vector3 rradius=wheels[i].arm->RelPosition-wheels[i].near_attach->RelPosition;
		Vector3 radius=Plane(axis, wheels[i].near_attach->RelPosition).projectVector(rradius);
		Real rlen=radius.length(); //length of the projected arm
		float offset=(rradius-radius).length(); //length of the error arm
		axis=total_torque*axis;
		if(rlen>0.01)
		{
			radius=radius/(2.0f*rlen*rlen);
			Vector3 cforce=axis.crossProduct(radius);
			//modulate the force according to induced torque error
			if (offset*2.0>rlen) cforce=Vector3::ZERO; // too much error!
			else cforce=(1.0f-((offset*2.0f)/rlen))*cforce; //linear modulation
			wheels[i].arm->Forces-=cforce;
			wheels[i].near_attach->Forces+=cforce;
		}
	}
	//LogManager::getSingleton().logMessage("torque "+StringConverter::toString(torques[0])+" "+StringConverter::toString(torques[1])+" "+StringConverter::toString(torques[2])+" "+StringConverter::toString(torques[3])+" speed "+StringConverter::toString(newspeeds[0])+" "+StringConverter::toString(newspeeds[1])+" "+StringConverter::toString(newspeeds[2])+" "+StringConverter::toString(newspeeds[3]));
	for (i=0; i<free_wheel; i++) wheels[i].speed=newspeeds[i];
	//wheel speed
	if (proped_wheels) wspeed/=(float)proped_wheels;
	lastwspeed=wspeed;
	WheelSpeed=wspeed;

	if(patchEngineTorque)
	{
		if (engine && free_wheel)
		{
			engine->setSpin(wspeed*9.549);
		}
	} else
	{
		if (engine && free_wheel && wheels[0].radius != 0)
		{
			engine->setSpin(wspeed*9.549/wheels[0].radius);
		}
	}

	BES_STOP(BES_CORE_Wheels);
	BES_START(BES_CORE_Shocks);

	//update position
//		if(free_node != 0)
//			aposition/=(Real)(free_node);
	//variable shocks for stabilisation
	if (free_active_shock && stabcommand)
	{
		if ((stabcommand==1 && stabratio<0.1) || (stabcommand==-1 && stabratio>-0.1))
			stabratio=stabratio+(float)stabcommand*dt*STAB_RATE;
		for (i=0; i<free_shock; i++)
		{
			// active shocks now
			if (shocks[i].flags & SHOCK_FLAG_RACTIVE)
				beams[shocks[i].beamid].L=beams[shocks[i].beamid].refL*(1.0+stabratio);
			else if (shocks[i].flags & SHOCK_FLAG_LACTIVE)
				beams[shocks[i].beamid].L=beams[shocks[i].beamid].refL*(1.0-stabratio);
		}
	}
	//auto shock adjust
	if (free_active_shock && doUpdate)
	{
		Vector3 dir=nodes[cameranodepos[0]].RelPosition-nodes[cameranoderoll[0]].RelPosition;
		dir.normalise();
		float roll=asin(dir.dotProduct(Vector3::UNIT_Y));
		//			mWindow->setDebugText("Roll:"+ StringConverter::toString(roll));
		if (fabs(roll)>0.2) stabsleep=-1.0; //emergency timeout stop
		if (fabs(roll)>0.03 && stabsleep<0.0)
		{
			if (roll>0.0 && stabcommand!=-1) stabcommand=1;
			else if (roll<0.0 && stabcommand!=1) stabcommand=-1; else {stabcommand=0;stabsleep=3.0;};
		}
		else stabcommand=0;

#ifdef USE_OPENAL
		if (stabcommand && fabs(stabratio)<0.1 && ssm)
			ssm->trigStart(trucknum, SS_TRIG_AIR);
		else if (ssm)
			ssm->trigStop(trucknum, SS_TRIG_AIR);
#endif //OPENAL

	}

	BES_STOP(BES_CORE_Shocks);
	BES_START(BES_CORE_Hydros);

	//direction
	if (hydrodirstate!=0 || hydrodircommand!=0)
	{
		float rate=1;
		if(hydroSpeedCoupling)
		{
			//new rate value (30 instead of 40) to deal with the changed cmdKeyInertia settings
			rate=30.0/(10.0+fabs(wspeed/2.0));
			// minimum rate: 20% --> enables to steer high velocity trucks
			if(rate<1.2) rate = 1.2;
		}
		//need a maximum rate for analog devices, otherwise hydro beams break
		if (!hydroSpeedCoupling)
			rate=10;
		if ( (hydrodircommand!=0 && hydroSpeedCoupling) || !hydroSpeedCoupling)
		{
			if (hydrodirstate > hydrodircommand)
				hydrodirstate -= dt * rate;
			else
				hydrodirstate += dt * rate;
		}
		if(hydroSpeedCoupling)
		{
			float dirdelta=dt;
			if      (hydrodirstate >  dirdelta) hydrodirstate -= dirdelta;
			else if (hydrodirstate < -dirdelta) hydrodirstate += dirdelta;
			else hydrodirstate=0;
		}
	}
	//aileron
	if (hydroaileronstate!=0 || hydroaileroncommand!=0)
	{
		if (hydroaileroncommand!=0)
		{
			if (hydroaileronstate>(hydroaileroncommand))
				hydroaileronstate-=dt*4.0;
			else
				hydroaileronstate+=dt*4.0;
		}
		float delta=dt;
		if (hydroaileronstate>delta) hydroaileronstate-=delta;
		else if (hydroaileronstate<-delta) hydroaileronstate+=delta;
		else hydroaileronstate=0;
	}
	//rudder
	if (hydrorudderstate!=0 || hydroruddercommand!=0)
	{
		if (hydroruddercommand!=0)
		{
			if (hydrorudderstate>(hydroruddercommand))
				hydrorudderstate-=dt*4.0;
			else
				hydrorudderstate+=dt*4.0;
		}

		float delta=dt;
		if (hydrorudderstate>delta) hydrorudderstate-=delta;
		else if (hydrorudderstate<-delta) hydrorudderstate+=delta;
		else hydrorudderstate=0;
	}
	//elevator
	if (hydroelevatorstate!=0 || hydroelevatorcommand!=0)
	{
		if (hydroelevatorcommand!=0)
		{
			if (hydroelevatorstate>(hydroelevatorcommand))
				hydroelevatorstate-=dt*4.0;
			else
				hydroelevatorstate+=dt*4.0;
		}
		float delta=dt;
		if (hydroelevatorstate>delta) hydroelevatorstate-=delta;
		else if (hydroelevatorstate<-delta) hydroelevatorstate+=delta;
		else hydroelevatorstate=0;
	}
	//update length, dirstate between -1.0 and 1.0
	for (int i=0; i<free_hydro; i++)
	{
		//compound hydro
		float cstate=0.0f;
		int div=0;
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_SPEED)
		{
			//special treatment for SPEED
			if (WheelSpeed<12.0)
				cstate += hydrodirstate*(12.0-WheelSpeed)/12.0;
			div++;
		}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_DIR) {cstate+=hydrodirstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_AILERON) {cstate+=hydroaileronstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_RUDDER) {cstate+=hydrorudderstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_ELEVATOR) {cstate+=hydroelevatorstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_REV_AILERON) {cstate-=hydroaileronstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_REV_RUDDER) {cstate-=hydrorudderstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_REV_ELEVATOR) {cstate-=hydroelevatorstate;div++;}

		if (cstate>1.0) cstate=1.0;
		if (cstate<-1.0) cstate=-1.0;
		// Animators following, if no animator, skip all the tests...
		int flagstate = beams[hydro[i]].animFlags;
		if(flagstate)
		{
			float animoption = beams[hydro[i]].animOption;
			calcAnimators(flagstate, cstate, div, dt, 0.0f, 0.0f, animoption);
			}

		if (div)
		{
			cstate=cstate/(float)div;
			if(hydroInertia)
				cstate=hydroInertia->calcCmdKeyDelay(cstate,i,dt);

			if (!(beams[hydro[i]].hydroFlags & HYDRO_FLAG_SPEED))
				hydrodirwheeldisplay=cstate;
			beams[hydro[i]].L=beams[hydro[i]].Lhydro*(1.0-cstate*beams[hydro[i]].hydroRatio);
		}
	}

	BES_STOP(BES_CORE_Hydros);
	BES_START(BES_CORE_Commands);


	// forward things to trailers
	if (state==ACTIVATED && forwardcommands)
	{
		int i,j;
		for (i=0; i<numtrucks; i++)
		{
			if(!trucks[i]) continue;
			if (trucks[i]->state==DESACTIVATED && trucks[i]->importcommands)
			{
				// forward commands
				for (j=1; j<MAX_COMMANDS; j++)
					trucks[i]->commandkey[j].commandValue = commandkey[j].commandValue;

				// just send brake and lights to the connected truck, and no one else :)
				for(std::vector<hook_t>::iterator it=hooks.begin(); it!=hooks.end(); it++)
				{
					if(!it->lockTruck) continue;
					// forward brake
					it->lockTruck->brake = brake;
					it->lockTruck->parkingbrake = parkingbrake;

					// forward lights
					it->lockTruck->lights = lights;
					it->lockTruck->blinkingtype = blinkingtype;
					//for(int k=0;k<4;k++)
					//	lockTruck->setCustomLight(k, getCustomLight(k));
					//forward reverse light e.g. for trailers
					it->lockTruck->reverselight=getReverseLightVisible();
				}
			}
		}
	}

	// commands
	if (hascommands)
	{
		int active=0;
		int requested=0;
		float work=0.0;
		if (engine)
			canwork=(engine->getRPM()>800.0);
		else
			canwork=1;
		float crankfactor=1;
		if (engine) crankfactor=engine->getCrankFactor();

		// speed up machines
		if(driveable==MACHINE)
			crankfactor = 2;

		for (i=0; i<=MAX_COMMANDS; i++)
			for (int j=0; j < (int)commandkey[i].beams.size(); j++)
				beams[abs(commandkey[i].beams[j])].autoMoveLock=false;

		for (i=0; i<=MAX_COMMANDS; i++)
			for (int j=0; j < (int)commandkey[i].beams.size(); j++)
				if(commandkey[i].commandValue >= 0.5)
					beams[abs(commandkey[i].beams[j])].autoMoveLock=true;


		// only process ties if there is enough force available
		if(canwork)
		{
			bool requestpower = false;
			// go through all ties and process them
			for(std::vector<tie_t>::iterator it=ties.begin(); it!=ties.end(); it++)
			{
				// only process tying ties
				if(!it->tying) continue;

				// division through zero guard
				if(it->beam->refL == 0 || it->beam->L == 0) continue;

				float clen = it->beam->L / it->beam->refL;
				if (clen > it->beam->commandShort)
				{
					float dl = it->beam->L;
					it->beam->L *= (1.0 - it->beam->commandRatioShort * dt / it->beam->L);
					dl = fabs(dl - it->beam->L);
					requestpower = true;
					active++;
					work += fabs(it->beam->stress) * dl;
				} else
				{
					// tying finished, end reached
					it->tying = false;
				}

				// check if we hit a certain force limit, then abort the tying process
				if (fabs(it->beam->stress) > it->beam->maxtiestress)
					it->tying = false;
			}
			if(requestpower)
				requested++;
		}

		// now process normal commands
		for (i=0; i<=MAX_COMMANDS; i++)
		{
			int j;

			bool requestpower = false;
			for (j=0; j < (int)commandkey[i].beams.size(); j++)
			{
				int bbeam=commandkey[i].beams[j];
				int bbeam_abs=abs(bbeam);

				// restrict forces
				if(beams[bbeam_abs].isforcerestricted && crankfactor > 1)
					crankfactor=1;

				float v = commandkey[i].commandValue;
				/*
				if(i==1)
				LogManager::getSingleton().logMessage(StringConverter::toString(v) + "/" + StringConverter::toString(beams[bbeam].autoMovingMode));
				*/

				// self centering
				if(beams[bbeam_abs].iscentering && !beams[bbeam_abs].autoMoveLock)
				{
					// check for some error
					if(beams[bbeam_abs].refL == 0 || beams[bbeam_abs].L == 0)
						continue;

					float current = (beams[bbeam_abs].L/beams[bbeam_abs].refL);
					/*
					if(i==1)
					LogManager::getSingleton().logMessage("centering: "+ \
					StringConverter::toString(current)+" / "+ \
					StringConverter::toString(beams[bbeam_abs].centerLength)+ " / " + \
					StringConverter::toString(v)+" / ");
					*/

					// hold condition
					if(fabs(current-beams[bbeam_abs].centerLength) < 0.0001)
					{
						beams[bbeam_abs].autoMovingMode = 0;
						/*
						if(i==1)
						LogManager::getSingleton().logMessage("centering complete");
						*/
					}
					else
					{
						// determine direction
						if(current > beams[bbeam_abs].centerLength)
							beams[bbeam_abs].autoMovingMode = -1;
						else
							beams[bbeam_abs].autoMovingMode = 1;
					}
				}

				if(beams[bbeam_abs].refL != 0 && beams[bbeam_abs].L != 0)
				{
					if (bbeam>0)
					{
						float clen = beams[bbeam].L/beams[bbeam].refL;
						if (clen<beams[bbeam].commandLong)
						{
							float dl=beams[bbeam].L;

							if(beams[bbeam].isOnePressMode==2)
							{
								// one press + centering
								//String sMode = (beams[bbeam].pressedCenterMode?"YES":"NO");
								//LogManager::getSingleton().logMessage(sMode+"|"+StringConverter::toString(clen)+" / "+StringConverter::toString(beams[bbeam].centerLength));
								if(beams[bbeam].autoMovingMode > 0 && clen > beams[bbeam].centerLength && !beams[bbeam].pressedCenterMode)
								{
									beams[bbeam].pressedCenterMode = true;
									beams[bbeam].autoMovingMode=0;
								}
								else if(beams[bbeam].autoMovingMode < 0 && clen > beams[bbeam].centerLength && beams[bbeam].pressedCenterMode)
									beams[bbeam].pressedCenterMode = false;
							}
							if(beams[bbeam].isOnePressMode>0)
							{
								bool key = (v > 0.5);
								if(beams[bbeam].autoMovingMode <= 0 && key)
								{
									//LogManager::getSingleton().logMessage("LONG auto-moving-start!");
									beams[bbeam].autoMovingMode=1;
								}
								else if(beams[bbeam].autoMovingMode==1 && !key)
								{
									//LogManager::getSingleton().logMessage("LONG auto-moving step2!");
									beams[bbeam].autoMovingMode=2;
								}
								else if(beams[bbeam].autoMovingMode==2 && key)
								{
									//LogManager::getSingleton().logMessage("LONG auto-moving-end step1!");
									beams[bbeam].autoMovingMode=3;
								}
								else if(beams[bbeam].autoMovingMode==3 && !key)
								{
									//LogManager::getSingleton().logMessage("LONG auto-moving-end step2!");
									beams[bbeam].autoMovingMode=0;
								}
							}

							if(cmdInertia)
								v=cmdInertia->calcCmdKeyDelay(v,i,dt);

							if(beams[bbeam].autoMovingMode > 0)
								v = 1;

							if(v>0.5)
								requestpower=true;

							if(!canwork)
								continue;

							beams[bbeam].L *= (1.0 + beams[bbeam].commandRatioLong * v * crankfactor * dt / beams[bbeam].L);;
							dl=fabs(dl-beams[bbeam].L);
							if(v>0.5)
							{
								active++;
								work+=fabs(beams[bbeam].stress)*dl;
							}
						} else
						{
							// beyond lenght
							if(beams[bbeam].isOnePressMode>0 && beams[bbeam].autoMovingMode > 0)
							{
								//LogManager::getSingleton().logMessage("LONG auto-moving-end!");
								beams[bbeam].autoMovingMode=0;
							}
						}
					} else
					{
						bbeam=-bbeam;
						float clen = beams[bbeam].L/beams[bbeam].refL;
						if (clen>beams[bbeam].commandShort)
						{
							float dl=beams[bbeam].L;

							if(beams[bbeam].isOnePressMode==2)
							{
								// one press + centering
								//String sMode = (beams[bbeam].pressedCenterMode?"YES":"NO");
								//LogManager::getSingleton().logMessage(sMode+"|"+StringConverter::toString(clen)+" / "+StringConverter::toString(beams[bbeam].centerLength));
								if(beams[bbeam].autoMovingMode < 0 && clen < beams[bbeam].centerLength && !beams[bbeam].pressedCenterMode)
								{
									beams[bbeam].pressedCenterMode = true;
									beams[bbeam].autoMovingMode=0;
								}
								else if(beams[bbeam].autoMovingMode > 0 && clen < beams[bbeam].centerLength && beams[bbeam].pressedCenterMode)
									beams[bbeam].pressedCenterMode = false;
							}
							if(beams[bbeam].isOnePressMode>0)
							{
								bool key = (v > 0.5);
								if(beams[bbeam].autoMovingMode >=0 && key)
								{
									//LogManager::getSingleton().logMessage("SHORT auto-moving-start!");
									beams[bbeam].autoMovingMode=-1;
								}
								else if(beams[bbeam].autoMovingMode==-1 && !key)
								{
									//LogManager::getSingleton().logMessage("SHORT auto-moving step2!");
									beams[bbeam].autoMovingMode=-2;
								}
								else if(beams[bbeam].autoMovingMode==-2 && key)
								{
									//LogManager::getSingleton().logMessage("SHORT auto-moving-end step1!");
									beams[bbeam].autoMovingMode=-3;
								}
								else if(beams[bbeam].autoMovingMode==-3 && !key)
								{
									//LogManager::getSingleton().logMessage("SHORT auto-moving-end step2!");
									beams[bbeam].autoMovingMode=0;
								}
							}

							if(cmdInertia)
								v=cmdInertia->calcCmdKeyDelay(v,i,dt);

							if(beams[bbeam].autoMovingMode < 0)
								v = 1;

							if(v>0.5)
								requestpower=true;

							if(!canwork)
								continue;

							beams[bbeam].L *= (1.0 - beams[bbeam].commandRatioShort * v * crankfactor * dt / beams[bbeam].L);
							dl=fabs(dl-beams[bbeam].L);
							if(v>0.5)
							{
								requestpower=true;
								active++;
								work+=fabs(beams[bbeam].stress)*dl;
							}
						} else
						{
							// beyond lenght
							if(beams[bbeam].isOnePressMode>0 && beams[bbeam].autoMovingMode < 0)
							{
								//LogManager::getSingleton().logMessage("SHORT auto-moving-end!");
								beams[bbeam].autoMovingMode=0;
							}
						}
					};
				}
			}
			//also for rotators
			for (j=0; j < (int)commandkey[i].rotators.size(); j++)
			{
				if ((commandkey[i].rotators[j])>0)
				{
					int rota = commandkey[i].rotators[j] - 1;
					float value=0;
					if(rotaInertia)
					{
						value=rotaInertia->calcCmdKeyDelay(commandkey[i].commandValue,i,dt);
					}
					if(value>0.5f)
						requestpower=true;
					rotators[rota].angle += rotators[rota].rate * value * crankfactor * dt;
				}
				else
				{
					int rota =- (commandkey[i].rotators[j]) - 1;
					float value=0;
					if(rotaInertia)
					{
						value=rotaInertia->calcCmdKeyDelay(commandkey[i].commandValue,i,dt);
					}
					if(value>0.5f)
						requestpower=true;
					rotators[rota].angle -= rotators[rota].rate * value * crankfactor * dt;
				}
			}
			if(requestpower)
				requested++;

		}

		if (engine)
		{
			engine->hydropump=work;
			engine->prime=requested;
		}
		if (doUpdate && state==ACTIVATED)
		{
#ifdef USE_OPENAL
			if (active && ssm)
			{
				ssm->trigStart(trucknum, SS_TRIG_PUMP);
				float pump_rpm=660.0*(1.0-(work/(float)active)/100.0);
				ssm->modulate(trucknum, SS_MOD_PUMP, pump_rpm);
			} else if(ssm)
				ssm->trigStop(trucknum, SS_TRIG_PUMP);
#endif //OPENAL
		}
		//rotators
		for (i=0; i<free_rotator; i++)
		{
			//compute rotation axis
			Vector3 axis=nodes[rotators[i].axis1].RelPosition-nodes[rotators[i].axis2].RelPosition;
			//axis.normalise();
			axis=fast_normalise(axis);
			//find the reference plane
			Plane pl=Plane(axis, 0);
			//for each pair
			int k;
			for (k=0; k<2; k++)
			{
				//find the reference vectors
				Vector3 ref1=pl.projectVector(nodes[rotators[i].axis2].RelPosition-nodes[rotators[i].nodes1[k]].RelPosition);
				Vector3 ref2=pl.projectVector(nodes[rotators[i].axis2].RelPosition-nodes[rotators[i].nodes2[k]].RelPosition);
				//theory vector
				Vector3 th1=Quaternion(Radian(rotators[i].angle+3.14159/2.0), axis)*ref1;
				//find the angle error
				float aerror=asin((th1.normalisedCopy()).dotProduct(ref2.normalisedCopy()));
				//			mWindow->setDebugText("Error:"+ StringConverter::toString(aerror));
				//exert forces
				float rigidity=10000000.0;
				Vector3 dir1=ref1.crossProduct(axis);
				//dir1.normalise();
				dir1=fast_normalise(dir1);
				Vector3 dir2=ref2.crossProduct(axis);
				//dir2.normalise();
				dir2=fast_normalise(dir2);
				float ref1len=ref1.length();
				float ref2len=ref2.length();
				nodes[rotators[i].nodes1[k]].Forces+=(aerror*ref1len*rigidity)*dir1;
				nodes[rotators[i].nodes2[k]].Forces-=(aerror*ref2len*rigidity)*dir2;
				//symmetric
				nodes[rotators[i].nodes1[k+2]].Forces-=(aerror*ref1len*rigidity)*dir1;
				nodes[rotators[i].nodes2[k+2]].Forces+=(aerror*ref2len*rigidity)*dir2;
			}
		}

	}

	BES_STOP(BES_CORE_Commands);
	BES_START(BES_CORE_Replay);

	//we also store a new replay frame
	if(replay)
	{
		replayTimer += dt;
		if(replayTimer > replayPrecision)
		{
			// store nodes
			node_simple_t *nbuff = (node_simple_t *)replay->getWriteBuffer(0);
			for (i=0; i<free_node; i++)
				nbuff[i].pos = nodes[i].AbsPosition;

			// store beams
			beam_simple_t *bbuff = (beam_simple_t *)replay->getWriteBuffer(1);
			for (i=0; i<free_beam; i++)
			{
				bbuff[i].scale = beams[i].scale;
				bbuff[i].broken = beams[i].broken;
				bbuff[i].disabled = beams[i].disabled;
			}

			replay->writeDone();
			replayTimer = 0.0f;
		}
	}

	BES_STOP(BES_CORE_Replay);
	BES_STOP(BES_CORE_WholeTruckCalc);

}
