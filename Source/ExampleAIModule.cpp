#include "ExampleAIModule.h" 
using namespace BWAPI;

bool analyzed;
bool analysis_just_finished;
BWTA::Region* home;
BWTA::Region* enemy_base;

ExampleAIModule::ExampleAIModule()
{
	Broodwar->printf("Initaion of AI");
	this->AIstate = 1;
	this->localCount = 0;
	this->currBuilder = NULL;
	this->currBuilding = NULL;
}

//This is the startup method. It is called once
//when a new game has been started with the bot.
void ExampleAIModule::onStart()
{

	//Enable flags
	Broodwar->enableFlag(Flag::UserInput);
	//Uncomment to enable complete map information
	Broodwar->enableFlag(Flag::CompleteMapInformation);

	//Start analyzing map data
	BWTA::readMap();
	analyzed=false;
	analysis_just_finished=false;
	//CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AnalyzeThread, NULL, 0, NULL); //Threaded version
	AnalyzeThread();
	
	this->createBuildPositions();

	for(int i = 0; i < 16; i++){
		Broodwar->printf("Build position %d: (%d, %d)", i, this->buildPos[i].x(), this->buildPos[i].y());
	}
    //Send each worker to the mineral field that is closest to it
    for(std::set<Unit*>::const_iterator i=Broodwar->self()->getUnits().begin();i!=Broodwar->self()->getUnits().end();i++)
    {

		if ((*i)->getType().isWorker())
		{	
			
			Unit* closestMineral=NULL;
			for(std::set<Unit*>::iterator m=Broodwar->getMinerals().begin();m!=Broodwar->getMinerals().end();m++)
			{
				if (closestMineral==NULL || (*i)->getDistance(*m)<(*i)->getDistance(closestMineral))
				{	
					closestMineral=*m;
				}
			}
			if (closestMineral!=NULL)
			{
				(*i)->rightClick(closestMineral);
				//Broodwar->printf("Send worker %d to mineral %d", (*i)->getID(), closestMineral->getID());
				
			}
		}

	}
	
	//Order one of our workers to guard our chokepoint.
	//Iterate through the list of units.
	Broodwar->printf("Mining initiated");

	//CHEATS
	//Broodwar->sendText("show me the money");
	
	for(std::set<Unit*>::const_iterator i=Broodwar->self()->getUnits().begin();i!=Broodwar->self()->getUnits().end();i++)
    {
	//Check if unit is a worker.
		if ((*i)->getType().isWorker())
		{
			//Find guard point
			Position guardPoint = findGuardPoint();
			//Order the worker to move to the guard point
			(*i)->rightClick(guardPoint);
			//Only send the first worker.
			Broodwar->printf("Guarding initiated");
			break;
		}
	}

	Broodwar->printf("%d",this->builders.size());
	
}

//Called when a game is ended.
//No need to change this.
void ExampleAIModule::onEnd(bool isWinner)
{
	if (isWinner)
	{
		Broodwar->sendText("I won!");
	}
}

//Finds a guard point around the home base.
//A guard point is the center of a chokepoint surrounding
//the region containing the home base.
Position ExampleAIModule::findGuardPoint()
{
	//Get the chokepoints linked to our home region
	std::set<BWTA::Chokepoint*> chokepoints = home->getChokepoints();
	double min_length = 10000;
	BWTA::Chokepoint* choke = NULL;
	//Iterate through all chokepoints and look for the one with the smallest gap (least width)
	for(std::set<BWTA::Chokepoint*>::iterator c = chokepoints.begin(); c != chokepoints.end(); c++)
	{
		double length = (*c)->getWidth();
		if (length < min_length || choke==NULL)
		{
			min_length = length;
			choke = *c;
		}
	}

	return choke->getCenter();
}

//This is the method called each frame. This is where the bot's logic
//shall be called.
void ExampleAIModule::onFrame()
{
	//Call every 100:th frame
	if (Broodwar->getFrameCount() % 50 == 0)
	{
		bool ordersGiven = false;
		bool noEnemy = false;
		std::set<Unit*> unitsInRange;
		Broodwar->printf("Current state: %d", this->AIstate);
		Broodwar->printf("Local Counter: %d", this->localCount);
		switch(this->AIstate){
			case 1:
				if(this->currBuilder != NULL && this->currBuilder->getOrder() == BWAPI::Orders::PlaceBuilding){
					ordersGiven = true;
					//Broodwar->printf("Adding one extra..!..!..!");
				}
				if(this->nrOfBuildnings(BWAPI::UnitTypes::Terran_Supply_Depot) == 2){
					this->AIstate = 2;
					this->currBuilder = NULL;
					break;
				}
				this->currBuilder = this->getBuilder();
				if(!ordersGiven && Broodwar->self()->minerals() > 100 && this->currBuilder != NULL){
					this->currBuilder->build(this->getNextBuildPosition(BWAPI::UnitTypes::Terran_Supply_Depot), BWAPI::UnitTypes::Terran_Supply_Depot);
				}
				break;


			case 2:
				if(this->currBuilder != NULL && this->currBuilder->getOrder() == BWAPI::Orders::PlaceBuilding){
					ordersGiven = true;
					//Broodwar->printf("Adding one extra..!..!..!");
				}
				if(this->nrOfBuildnings(BWAPI::UnitTypes::Terran_Barracks) == 1){
					this->AIstate = 3;
					this->currBuilder = NULL;
					break;
				}
				this->currBuilder = this->getBuilder();
				if(!ordersGiven && Broodwar->self()->minerals() > 150 && this->currBuilder != NULL){
					this->currBuilder->build(this->getNextBuildPosition(BWAPI::UnitTypes::Terran_Barracks), BWAPI::UnitTypes::Terran_Barracks);
				}
				break;


			case 3:
				if(this->localCount == 10){
					this->AIstate = 4;
					this->localCount = 0;
					break;
				}
				this->currBuilding = this->getBuilding(BWAPI::UnitTypes::Terran_Barracks);
				if(this->currBuilding != NULL && this->currBuilding->getRallyPosition() != this->createRallyPoint()){
					this->currBuilding->setRallyPoint(this->createRallyPoint());
				}
				if(Broodwar->self()->minerals() > 50 && this->currBuilding != NULL){
					if(this->currBuilding->train(BWAPI::UnitTypes::Terran_Marine)){
						this->localCount++;
					}
				}
				break;


			case 4:
				
				if(this->localCount == 5){
					this->AIstate = 5;
					this->localCount = 0;
					break;
				}
				this->currBuilding = this->getBuilding(BWAPI::UnitTypes::Terran_Command_Center);
				if(Broodwar->self()->minerals() > 50 && this->currBuilding != NULL){
					if(this->currBuilding->train(BWAPI::UnitTypes::Terran_SCV)){
						this->localCount++;
					}
				}
				break;


			case 5:
				if(this->currBuilder != NULL && this->currBuilder->getOrder() == BWAPI::Orders::PlaceBuilding){
					ordersGiven = true;
					Broodwar->printf("Adding one extra..!..!..!");
				}
				Broodwar->printf("Nr of refienerys: %d", this->nrOfBuildnings(BWAPI::UnitTypes::Terran_Refinery));
				if(this->nrOfBuildnings(BWAPI::UnitTypes::Terran_Refinery) == 1){
					this->AIstate = 6;
					this->currBuilder = NULL;
					break;
				}
				this->currBuilder = this->getBuilder();
				if(!ordersGiven && Broodwar->self()->minerals() > 100 && this->currBuilder != NULL){
					Unit* base = this->getBuilding(BWAPI::UnitTypes::Terran_Command_Center);
					Unit* closestGas=NULL;
					for(std::set<Unit*>::iterator g=Broodwar->getGeysers().begin();g!=Broodwar->getGeysers().end();g++)
					{
						if (closestGas==NULL || base->getDistance(*g) < base->getDistance(closestGas))
						{	
							closestGas=*g;
						}
					}
					if (closestGas!=NULL)
					{
						this->currBuilder->build(closestGas->getTilePosition(), BWAPI::UnitTypes::Terran_Refinery);
					}
				}
				break;
			case 6:
				this->localCount = 0;
				for(std::set<Unit*>::const_iterator i=this->builders.begin();i!=this->builders.end();i++)
				{
					//Broodwar->printf("Looking thrugh builders, curr id: %d", (*i)->getID());
					if((*i)->isGatheringGas())
					{
						this->localCount++;
					}
				}
				this->currBuilding = this->getBuilding(BWAPI::UnitTypes::Terran_Refinery);
				if(this->currBuilding != NULL && this->currBuilding->isCompleted() && this->localCount < 2){
					this->getBuilder()->rightClick(this->currBuilding);
				}
				else if(this->currBuilding != NULL && this->currBuilding->isCompleted()){
					this->AIstate = 7;
					this->localCount = 0;
					break;
				}
				break;

			case 7:
				if(this->currBuilder != NULL && this->currBuilder->getOrder() == BWAPI::Orders::PlaceBuilding){
					ordersGiven = true;
					//Broodwar->printf("Adding one extra..!..!..!");
				}
				if(this->nrOfBuildnings(BWAPI::UnitTypes::Terran_Academy) == 1){
					this->AIstate = 8;
					this->currBuilder = NULL;
					break;
				}
				this->currBuilder = this->getBuilder();
				if(!ordersGiven && Broodwar->self()->minerals() > 150 && this->currBuilder != NULL){
					this->currBuilder->build(this->getNextBuildPosition(BWAPI::UnitTypes::Terran_Academy), BWAPI::UnitTypes::Terran_Academy);
				}
				break;
			case 8:
				if(this->localCount == 3){
					this->AIstate = 9;
					this->localCount = 0;
					break;
				}
				this->currBuilding = this->getBuilding(BWAPI::UnitTypes::Terran_Barracks);
				if(this->currBuilding != NULL && this->currBuilding->getRallyPosition() != this->createRallyPoint()){
					this->currBuilding->setRallyPoint(this->createRallyPoint());
				}
				if(Broodwar->self()->minerals() > 50 && Broodwar->self()->gas() > 25 && this->currBuilding != NULL){
					if(this->currBuilding->train(BWAPI::UnitTypes::Terran_Medic)){
						this->localCount++;
					}
				}
				break;

			
			case 9:
				if(this->currBuilder != NULL && this->currBuilder->getOrder() == BWAPI::Orders::PlaceBuilding){
					ordersGiven = true;
					//Broodwar->printf("Adding one extra..!..!..!");
				}
				if(this->nrOfBuildnings(BWAPI::UnitTypes::Terran_Factory) == 1){
					this->AIstate = 10;
					this->currBuilder = NULL;
					break;
				}
				this->currBuilder = this->getBuilder();
				if(!ordersGiven && Broodwar->self()->minerals() > 200 && Broodwar->self()->gas() > 100 && this->currBuilder != NULL){
					this->currBuilder->build(this->buildPos[14], BWAPI::UnitTypes::Terran_Factory);
				}
				break;

			case 10:
				if(this->nrOfBuildnings(BWAPI::UnitTypes::Terran_Machine_Shop) == 1){
					this->AIstate = 11;
					this->currBuilder = NULL;
					break;
				}
				this->currBuilding = this->getBuilding(BWAPI::UnitTypes::Terran_Factory);
				if(Broodwar->self()->minerals() > 200 && Broodwar->self()->gas() > 100 && this->currBuilding != NULL){
					this->currBuilding->buildAddon(BWAPI::UnitTypes::Terran_Machine_Shop);
				}
				break;

			case 11:
				this->currBuilding = this->getBuilding(BWAPI::UnitTypes::Terran_Machine_Shop);
				if(Broodwar->self()->minerals() > 150 && Broodwar->self()->gas() > 150 && this->currBuilding != NULL){
					if(Broodwar->self()->isResearchAvailable(BWAPI::TechTypes::Tank_Siege_Mode)){
						if(this->currBuilding->research(BWAPI::TechTypes::Tank_Siege_Mode)){
							this->AIstate = 12;
							break;
						}
					}
				}
				break;

			case 12:
				if(this->currBuilder != NULL && this->currBuilder->getOrder() == BWAPI::Orders::PlaceBuilding){
					ordersGiven = true;
					//Broodwar->printf("Adding one extra..!..!..!");
				}
				if(this->nrOfBuildnings(BWAPI::UnitTypes::Terran_Supply_Depot) == 3){
					this->AIstate = 13;
					this->currBuilder = NULL;
					break;
				}
				this->currBuilder = this->getBuilder();
				if(!ordersGiven && Broodwar->self()->minerals() > 100 && this->currBuilder != NULL){
					this->currBuilder->build(this->getNextBuildPosition(BWAPI::UnitTypes::Terran_Supply_Depot), BWAPI::UnitTypes::Terran_Supply_Depot);
				}
				break;
			case 13:
				if(this->localCount == 3){
					this->AIstate = 14;
					this->localCount = 0;
					break;
				}
				this->currBuilding = this->getBuilding(BWAPI::UnitTypes::Terran_Factory);
				if(this->currBuilding != NULL && this->currBuilding->getRallyPosition() != this->findGuardPoint()){
					this->currBuilding->setRallyPoint(this->findGuardPoint());
				}
				if(Broodwar->self()->minerals() > 150 && Broodwar->self()->gas() > 100 && this->currBuilding != NULL){
					if(this->currBuilding->train(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode)){
						this->localCount++;
					}
				}
				break;
			case 14:
				if(this->localCount == 0){
					/*for(std::set<Unit*>::const_iterator i=this->army.begin();i!=this->army.end();i++)
					{
						(*i)->rightClick(enemy_base->getCenter());
						unitsInSight = (*i)->getUnitsInRadius(100);
						for(std::set<Unit*>::const_iterator j=this->unitsInSight.begin();j!=this->unitsInSight.end();j++)
						{
							if((*j)->getPlayer() != Broodwar->self()){
								(*i)->attack((*j));
							}
						}
					}*/

					
					for(std::set<Unit*>::const_iterator i=this->army.begin();i!=this->army.end();i++)
					{
						if((*i)->getType() == BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode){
							unitsInRange = (*i)->getUnitsInRadius(360); //Max range = 384
							Broodwar->printf("Tank Mode: Units in range size: %d", unitsInRange.size());
							for(std::set<Unit*>::const_iterator j=unitsInRange.begin();j!=unitsInRange.end();j++)
							{
								if((*j)->getPlayer() == Broodwar->enemy()){
									(*i)->siege();
									Broodwar->printf("Siege mode activated!");
									break;
								}
							}
						}
						if((*i)->getType() == BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode){
							unitsInRange = (*i)->getUnitsInRadius(360);
							noEnemy = true;
							Broodwar->printf("Siege Mode: Units in range size: %d", unitsInRange.size());
							for(std::set<Unit*>::const_iterator j=unitsInRange.begin();j!=unitsInRange.end();j++)
							{
								if((*j)->getPlayer() == Broodwar->enemy()){
									noEnemy = false;
									break;
								}
							}
							if(noEnemy){
								(*i)->unsiege();
								Broodwar->printf("Unsiege has been issued!");
							}
						}
						(*i)->attack(BWAPI::Position::Position(Broodwar->enemy()->getStartLocation()));
					}
					//this->localCount = 1;
				}
				break;
			default:
				Broodwar->printf("AIState fucked up!");
				break;
		}
		Unit* idleBuilder = this->getBuilder(true);
		if(idleBuilder != NULL){
			this->sendWorkerMine(idleBuilder);
		}

	}
  
	//Draw lines around regions, chokepoints etc.
	if (analyzed)
	{
		drawTerrainData();
		/*Broodwar->printf("Drawing building positions...");
		for(int i = 0; i < 10; i++){
			Broodwar->drawBox(CoordinateType::Map, 
				this->buildPos[i].x() * 32, this->buildPos[i].y() * 32, 
				(this->buildPos[i].x() + 4) * 32, (this->buildPos[i].y() + 4)* 32,
				Colors::Green, false);
		}*/
	}
}

//Is called when text is written in the console window.
//Can be used to toggle stuff on and off.
void ExampleAIModule::onSendText(std::string text)
{
	if (text=="/show players")
	{
		showPlayers();
	}
	else if (text=="/show forces")
	{
		showForces();
	}
	else
	{
		Broodwar->printf("You typed '%s'!",text.c_str());
		Broodwar->sendText("%s",text.c_str());
	}
}

//Called when the opponent sends text messages.
//No need to change this.
void ExampleAIModule::onReceiveText(BWAPI::Player* player, std::string text)
{
	Broodwar->printf("%s said '%s'", player->getName().c_str(), text.c_str());
}

//Called when a player leaves the game.
//No need to change this.
void ExampleAIModule::onPlayerLeft(BWAPI::Player* player)
{
	Broodwar->sendText("%s left the game.",player->getName().c_str());
}

//Called when a nuclear launch is detected.
//No need to change this.
void ExampleAIModule::onNukeDetect(BWAPI::Position target)
{
	if (target!=Positions::Unknown)
	{
		Broodwar->printf("Nuclear Launch Detected at (%d,%d)",target.x(),target.y());
	}
	else
	{
		Broodwar->printf("Nuclear Launch Detected");
	}
}

//No need to change this.
void ExampleAIModule::onUnitDiscover(BWAPI::Unit* unit)
{
	//Broodwar->sendText("A %s [%x] has been discovered at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x(),unit->getPosition().y());
}

//No need to change this.
void ExampleAIModule::onUnitEvade(BWAPI::Unit* unit)
{
	//Broodwar->sendText("A %s [%x] was last accessible at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x(),unit->getPosition().y());
}

//No need to change this.
void ExampleAIModule::onUnitShow(BWAPI::Unit* unit)
{
	//Broodwar->sendText("A %s [%x] has been spotted at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x(),unit->getPosition().y());
}

//No need to change this.
void ExampleAIModule::onUnitHide(BWAPI::Unit* unit)
{
	//Broodwar->sendText("A %s [%x] was last seen at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x(),unit->getPosition().y());
}

//Called when a new unit has been created.
//Note: The event is called when the new unit is built, not when it
//has been finished.
void ExampleAIModule::onUnitCreate(BWAPI::Unit* unit)
{
	if (unit->getPlayer() == Broodwar->self())
	{
		Broodwar->sendText("A %s [%x] has been created at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x(),unit->getPosition().y());
		this->underConstruction.insert(unit);
		Broodwar->printf("Added unit/building %d to underConstruction queue", unit->getID() );
		Broodwar->printf("%d",this->underConstruction.size());
	}
}

void ExampleAIModule::onUnitComplete(BWAPI::Unit* unit)
{
	if (unit->getPlayer() == Broodwar->self())
	{
		if(unit->getType().isWorker()){
			this->builders.insert(unit);
			Broodwar->printf("Added worker %d to builders", unit->getID() );
			Broodwar->printf("%d",this->builders.size());
		}
		else if (!unit->getType().isBuilding()){
			this->army.insert(unit);
			Broodwar->printf("Added soldier %d to army", unit->getID() );
			Broodwar->printf("%d",this->army.size());	
		}
		else{
			this->buildings.insert(unit);
			Broodwar->printf("Added building %d to buildings", unit->getID() );
			Broodwar->printf("%d",this->buildings.size());	
		}
		
		//Remove unit/building from construction queue
		this->underConstruction.erase(this->underConstruction.find(unit));
		Broodwar->printf("Deleted unit/Building from underConstruction queue");
		Broodwar->printf("%d",this->underConstruction.size());
	}
}

//Called when a unit has been destroyed.
void ExampleAIModule::onUnitDestroy(BWAPI::Unit* unit)
{
	if (unit->getPlayer() == Broodwar->self())
	{
		if(!unit->isCompleted()){	//Being constructed
			this->underConstruction.erase(this->underConstruction.find(unit));
			Broodwar->printf("Deleted unit/building from underConstruction queue");
			Broodwar->printf("%d",this->underConstruction.size());
		}
		else if(unit->getType().isWorker()){	//Is a worker
			this->builders.erase(this->builders.find(unit));
			Broodwar->printf("Deleted builder from builders");
			Broodwar->printf("%d",this->builders.size());

		}
		else if(unit->getType().isBuilding())
		{
			this->buildings.erase(this->buildings.find(unit));
			Broodwar->printf("Deleted building from buildings");
			Broodwar->printf("%d",this->buildings.size());
		} 
		else{
			this->army.erase(this->army.find(unit));
			Broodwar->printf("Deleted soldier from army");
			Broodwar->printf("%d",this->army.size());
		}
		
		Broodwar->sendText("My unit %s [%x] has been destroyed at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x(),unit->getPosition().y());
	}
	else
	{
		Broodwar->sendText("Enemy unit %s [%x] has been destroyed at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x(),unit->getPosition().y());
	}
}

BWAPI::Unit* ExampleAIModule::getBuilder(bool idleOnly){
	
	for(std::set<Unit*>::const_iterator i=this->builders.begin();i!=this->builders.end();i++)
    {
		//Broodwar->printf("Looking thrugh builders, curr id: %d", (*i)->getID());
		if((*i)->isIdle())
		{
			Broodwar->printf("Found idle ID: %d", (*i)->getID());
			return (*i);
		}
	}
	if(!idleOnly){
	// If no idle is found
		for(std::set<Unit*>::const_iterator i=this->builders.begin();i!=this->builders.end();i++)
		{
			//Broodwar->printf("No idle found.. Next, curr id: %d", (*i)->getID());
			if(!(*i)->isConstructing() && !(*i)->isGatheringGas())
			{
				Broodwar->printf("Found NON idle ID: %d", (*i)->getID());
				return (*i);
			}
		}
	}

	return NULL;
}

//Only needed for Zerg units.
//No need to change this.
void ExampleAIModule::onUnitMorph(BWAPI::Unit* unit)
{
	//Broodwar->sendText("A %s [%x] has been morphed at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x(),unit->getPosition().y());
}

//No need to change this.
void ExampleAIModule::onUnitRenegade(BWAPI::Unit* unit)
{
	if (unit->getPlayer() == Broodwar->self() && unit->getType() == BWAPI::UnitTypes::Terran_Refinery)
	{
		this->buildings.insert(unit);
		Broodwar->printf("Added building %d to buildings", unit->getID() );
		Broodwar->printf("%d",this->buildings.size());	
	}
	//Broodwar->sendText("A %s [%x] is now owned by %s",unit->getType().getName().c_str(),unit,unit->getPlayer()->getName().c_str());
}

//No need to change this.
void ExampleAIModule::onSaveGame(std::string gameName)
{
	Broodwar->printf("The game was saved to \"%s\".", gameName.c_str());
}

//Analyzes the map.
//No need to change this.
DWORD WINAPI AnalyzeThread()
{
	BWTA::analyze();

	//Self start location only available if the map has base locations
	if (BWTA::getStartLocation(BWAPI::Broodwar->self())!=NULL)
	{
		home = BWTA::getStartLocation(BWAPI::Broodwar->self())->getRegion();
	}
	//Enemy start location only available if Complete Map Information is enabled.
	if (BWTA::getStartLocation(BWAPI::Broodwar->enemy())!=NULL)
	{
		enemy_base = BWTA::getStartLocation(BWAPI::Broodwar->enemy())->getRegion();
	}
	analyzed = true;
	analysis_just_finished = true;
	return 0;
}

//Prints some stats about the units the player has.
//No need to change this.
void ExampleAIModule::drawStats()
{
	std::set<Unit*> myUnits = Broodwar->self()->getUnits();
	Broodwar->drawTextScreen(5,0,"I have %d units:",myUnits.size());
	std::map<UnitType, int> unitTypeCounts;
	for(std::set<Unit*>::iterator i=myUnits.begin();i!=myUnits.end();i++)
	{
		if (unitTypeCounts.find((*i)->getType())==unitTypeCounts.end())
		{
			unitTypeCounts.insert(std::make_pair((*i)->getType(),0));
		}
		unitTypeCounts.find((*i)->getType())->second++;
	}
	int line=1;
	for(std::map<UnitType,int>::iterator i=unitTypeCounts.begin();i!=unitTypeCounts.end();i++)
	{
		Broodwar->drawTextScreen(5,16*line,"- %d %ss",(*i).second, (*i).first.getName().c_str());
		line++;
	}
}

//Draws terrain data aroung regions and chokepoints.
//No need to change this.
void ExampleAIModule::drawTerrainData()
{
	//Iterate through all the base locations, and draw their outlines.
	for(std::set<BWTA::BaseLocation*>::const_iterator i=BWTA::getBaseLocations().begin();i!=BWTA::getBaseLocations().end();i++)
	{
		TilePosition p=(*i)->getTilePosition();
		Position c=(*i)->getPosition();
		//Draw outline of center location
		Broodwar->drawBox(CoordinateType::Map,p.x()*32,p.y()*32,p.x()*32+4*32,p.y()*32+3*32,Colors::Blue,false);
		//Draw a circle at each mineral patch
		for(std::set<BWAPI::Unit*>::const_iterator j=(*i)->getStaticMinerals().begin();j!=(*i)->getStaticMinerals().end();j++)
		{
			Position q=(*j)->getInitialPosition();
			Broodwar->drawCircle(CoordinateType::Map,q.x(),q.y(),30,Colors::Cyan,false);
		}
		//Draw the outlines of vespene geysers
		for(std::set<BWAPI::Unit*>::const_iterator j=(*i)->getGeysers().begin();j!=(*i)->getGeysers().end();j++)
		{
			TilePosition q=(*j)->getInitialTilePosition();
			Broodwar->drawBox(CoordinateType::Map,q.x()*32,q.y()*32,q.x()*32+4*32,q.y()*32+2*32,Colors::Orange,false);
		}
		//If this is an island expansion, draw a yellow circle around the base location
		if ((*i)->isIsland())
		{
			Broodwar->drawCircle(CoordinateType::Map,c.x(),c.y(),80,Colors::Yellow,false);
		}
	}
	//Iterate through all the regions and draw the polygon outline of it in green.
	for(std::set<BWTA::Region*>::const_iterator r=BWTA::getRegions().begin();r!=BWTA::getRegions().end();r++)
	{
		BWTA::Polygon p=(*r)->getPolygon();
		for(int j=0;j<(int)p.size();j++)
		{
			Position point1=p[j];
			Position point2=p[(j+1) % p.size()];
			Broodwar->drawLine(CoordinateType::Map,point1.x(),point1.y(),point2.x(),point2.y(),Colors::Green);
		}
	}
	//Visualize the chokepoints with red lines
	for(std::set<BWTA::Region*>::const_iterator r=BWTA::getRegions().begin();r!=BWTA::getRegions().end();r++)
	{
		for(std::set<BWTA::Chokepoint*>::const_iterator c=(*r)->getChokepoints().begin();c!=(*r)->getChokepoints().end();c++)
		{
			Position point1=(*c)->getSides().first;
			Position point2=(*c)->getSides().second;
			Broodwar->drawLine(CoordinateType::Map,point1.x(),point1.y(),point2.x(),point2.y(),Colors::Red);
		}
	}
	//Draw the building positions
	//Broodwar->printf("Drawing building positions...");
	for(int i = 0; i < 16; i++){
		if(Broodwar->isBuildable(this->buildPos[i])){
			Broodwar->drawBox(CoordinateType::Map, 
			this->buildPos[i].x() * 32, this->buildPos[i].y() * 32, 
			(this->buildPos[i].x() + 4) * 32, (this->buildPos[i].y() + 4)* 32,
			Colors::Purple, false);
		}
		else{
			Broodwar->drawBox(CoordinateType::Map, 
			this->buildPos[i].x() * 32, this->buildPos[i].y() * 32, 
			(this->buildPos[i].x() + 4) * 32, (this->buildPos[i].y() + 4)* 32,
			Colors::Orange, false);
		}
	}
}

//Show player information.
//No need to change this.
void ExampleAIModule::showPlayers()
{
	std::set<Player*> players=Broodwar->getPlayers();
	for(std::set<Player*>::iterator i=players.begin();i!=players.end();i++)
	{
		Broodwar->printf("Player [%d]: %s is in force: %s",(*i)->getID(),(*i)->getName().c_str(), (*i)->getForce()->getName().c_str());
	}
}

//Show forces information.
//No need to change this.
void ExampleAIModule::showForces()
{
	std::set<Force*> forces=Broodwar->getForces();
	for(std::set<Force*>::iterator i=forces.begin();i!=forces.end();i++)
	{
		std::set<Player*> players=(*i)->getPlayers();
		Broodwar->printf("Force %s has the following players:",(*i)->getName().c_str());
		for(std::set<Player*>::iterator j=players.begin();j!=players.end();j++)
		{
			Broodwar->printf("  - Player [%d]: %s",(*j)->getID(),(*j)->getName().c_str());
		}
	}
}

void ExampleAIModule::createBuildPositions()
{
	TilePosition baseTile = Broodwar->self()->getStartLocation();
	TilePosition chokeTile = TilePosition(this->findGuardPoint());
	int xs[16];
	int ys[16];
	if(baseTile.x() < chokeTile.x()){
		for(int i = 0; i < 16; i++){
			xs[i] = baseTile.x() + ((i / 4) * 4);
		}
	}
	else{
		for(int i = 0; i < 16; i++){
			xs[i] = baseTile.x() - ((i / 4) * 4);
		}
	}

	if(baseTile.y() < chokeTile.y()){
		for(int i = 0; i < 16; i++){
			ys[i] = baseTile.y() + ((i % 4) * 4);
		}
	}
	else{
		for(int i = 0; i < 16; i++){
			ys[i] = baseTile.y() - ((i % 4) * 4);
		}
	}

	for(int i = 0; i < 16; i++){
		this->buildPos[i] = TilePosition(xs[i], ys[i]);
	}
}

BWAPI::TilePosition ExampleAIModule::getNextBuildPosition(BWAPI::UnitType buildingType)
{
	Unit* builder = this->getBuilder();
	for(int i = 0; i < 16; i++){
		if(Broodwar->canBuildHere(builder, this->buildPos[i], buildingType)){
			Broodwar->printf("Got the buildtile id: %d", i);
			return BWAPI::TilePosition(this->buildPos[i]);
		}
	}
	return this->buildPos[0];
}

int ExampleAIModule::nrOfBuildnings(BWAPI::UnitType buildingType)
{
	int counter = 0;
	for(std::set<Unit*>::iterator i=this->buildings.begin();i!=this->buildings.end();i++)
	{
		if((*i)->getType() == buildingType){
			counter++;
		}
	}
	for(std::set<Unit*>::iterator i=this->underConstruction.begin();i!=this->underConstruction.end();i++)
	{
		if((*i)->getType() == buildingType){
			counter++;
		}
	}
	return counter;
}

int ExampleAIModule::nrOfUnits(BWAPI::UnitType unitType)
{
	int counter = 0;
	for(std::set<Unit*>::iterator i=this->army.begin();i!=this->army.end();i++)
	{
		if((*i)->getType() == unitType){
			counter++;
		}
	}
	return counter;
}

BWAPI::Unit* ExampleAIModule::getBuilding(BWAPI::UnitType buildingType)
{
	for(std::set<Unit*>::iterator i=this->buildings.begin();i!=this->buildings.end();i++)
	{
		if((*i)->getType() == buildingType){
			return (*i);
		}
	}
	return NULL;
}

void ExampleAIModule::sendWorkerMine(BWAPI::Unit* worker)
{
	Unit* closestMineral=NULL;
	Unit* base = this->getBuilding(BWAPI::UnitTypes::Terran_Command_Center);
	if(base != NULL){
		for(std::set<Unit*>::iterator m=Broodwar->getMinerals().begin();m!=Broodwar->getMinerals().end();m++)
		{
			if (closestMineral==NULL || base->getDistance(*m) < base->getDistance(closestMineral))
			{	
				closestMineral=*m;
			}
		}
		if (closestMineral != NULL)
		{
			worker->rightClick(closestMineral);
			//Broodwar->printf("Send worker %d to mineral %d", (*i)->getID(), closestMineral->getID());
			
		}
	}
}

BWAPI::Position ExampleAIModule::createRallyPoint()
{
	BWAPI::TilePosition baseTile = Broodwar->self()->getStartLocation();
	BWAPI::Position chokePos = this->findGuardPoint();
	int x = 0;
	int y = 0;
	if(baseTile.x() < chokePos.x()){
		x = chokePos.x() - 50;
	}
	else{
		x = chokePos.x() + 50;
	}
	if(baseTile.y() < chokePos.y()){
		y = chokePos.y() - 20;
	}
	else{
		y = chokePos.y() + 20;
	}

	return BWAPI::Position::Position(x, y);
}