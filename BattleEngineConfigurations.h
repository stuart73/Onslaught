#ifndef BATTLEENGINECONFIGURATIONS_H
#define BATTLEENGINECONFIGURATIONS_H

#include	"BattleEngineDataManager.h"
#include	"membuffer.h"

static const SINT kMaxConfigurations=20;

class UBattleEngineConfigurations
{
	private:
		static SLONG				sConfigurations;
		static char					*sConfigurationName[kMaxConfigurations];

	public:

		static void					Initialise();
		static void					ShutDown();

		static void					Load( CMEMBUFFER &inFile );
		static void					Skip( CMEMBUFFER &inFile );

		static CBattleEngineData*	GetConfiguration(
										int			inConfigurationId);

		static SLONG				CountConfigurations()				{ return sConfigurations; }
};

#endif