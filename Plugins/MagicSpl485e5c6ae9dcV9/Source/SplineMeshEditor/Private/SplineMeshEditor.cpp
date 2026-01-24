// Copyright 2022 Triple Scale Games. All Rights Reserved.

#include "SplineMeshEditor.h"
#include "SME_SplineMeshActor.h"
#include "SME_SplineLayer.h"
#include "SME_Segment.h"

#define LOCTEXT_NAMESPACE "FSplineMeshEditorModule"

void FSplineMeshEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	ASME_SplineMeshActor::StaticClass();
	USME_SplineLayer::StaticClass();
	USME_Segment::StaticClass();
}

void FSplineMeshEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSplineMeshEditorModule, SplineMeshEditor)