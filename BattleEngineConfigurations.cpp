#include "Common.h"
#include "BattleEngineConfigurations.h"
#include "Console.h"

SLONG	UBattleEngineConfigurations::sConfigurations;
char	*UBattleEngineConfigurations::sConfigurationName[kMaxConfigurations];

void UBattleEngineConfigurations::Initialise()
{
	sConfigurations=0;

	for (int n=0; n<kMaxConfigurations; n++)
		sConfigurationName[n]=0L;
}

void UBattleEngineConfigurations::ShutDown()
{
	sConfigurations=0;

	for (int n=0; n<kMaxConfigurations; n++)
	{
		if (sConfigurationName[n])
		{
			delete [] sConfigurationName[n];
			sConfigurationName[n]=0L;
		}
	}
}

void UBattleEngineConfigurations::Load( CMEMBUFFER &inFile )
{
	ShutDown();
	
	CONSOLE.Status("Loading battle engine configurations");

	inFile.Read(&sConfigurations,sizeof(sConfigurations));

	for (SINT n=0; n<sConfigurations; n++)
	{
		char	length;
		inFile.Read(&length,sizeof(length));

		// Load the configuration name
		sConfigurationName[n]=new( MEMTYPE_BATTLEENGINE ) char[length+1];

		int c;
		for (c=0; c<length; c++)
			inFile.Read(&sConfigurationName[n][c],sizeof(sConfigurationName[n][c]));

		sConfigurationName[n][c]=0;
	}

	CONSOLE.StatusDone("Loading battle engine configurations");	
}

void UBattleEngineConfigurations::Skip( CMEMBUFFER &inFile )
{
	SLONG		configurations;

	inFile.Read(&configurations,sizeof(configurations));

	for (SINT n=0; n<configurations; n++)
	{
		char	length;
		inFile.Read(&length,sizeof(length));

		// Load the configuration name
		char	*name=new( MEMTYPE_BATTLEENGINE ) char[length+1];

		SINT c;
		for (c=0; c<length; c++)
			inFile.Read(&name[c],sizeof(name[c]));

		name[c]=0;

		delete [] name;
	}
}

CBattleEngineData* UBattleEngineConfigurations::GetConfiguration(
	int			inConfigurationId)
{
	if ((inConfigurationId<0) || (inConfigurationId>=sConfigurations))
		inConfigurationId=0;

	CBattleEngineData	*data;
	
	if (data=UBattleEngineDataManager::GetConfiguration(sConfigurationName[inConfigurationId]))
		return data;

	return UBattleEngineDataManager::GetConfiguration(0);
}