// Copyright 2022 Triple Scale Games. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FSplineMeshEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
