// Fill out your copyright notice in the Description page of Project Settings.


#include "HeightSpan.h"

UHeightSpan::UHeightSpan(const int MinHeight, const int MaxHeight, const PolygonType& Type)
	: Min(MinHeight),
	  Max(MaxHeight),
	  SpanAttribute(Type)
{
}

void UHeightSpan::SetMaxHeight(const int NewHeight)
{
	if (NewHeight <= Min)
	{
		Max = Min + 1;
	}
	else
	{
		Max = NewHeight;
	}
}

void UHeightSpan::SetMinHeight(const int NewHeight)
{
	if (NewHeight >= Max)
	{
		Min = Max - 1;
	}
	else
	{
		Min = NewHeight;
	}
}

