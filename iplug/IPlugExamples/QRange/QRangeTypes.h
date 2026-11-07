#pragma once

enum EFilterTypes
{
	kTypePeak,
	kTypeLoShelf,
	kTypeHiShelf,
	kTypeLoCut,
	kTypeHiCut,

	kNumFilterTypes
};

enum ERoutings
{
	kRoutingStereo,
	kRoutingLeft,
	kRoutingRight,
	kRoutingMid,
	kRoutingSide,

	kNumRoutings
};

enum ECutOrders
{
    kCutOrder12dB,
    kCutOrder24dB,
    kCutOrder48dB,

    kNumCutOrders
};

enum ELatencySelection
{
    kLatencyIIR,
    kLatencyShort,
    kLatencyMedium,
    kLatencyLong,
    kLatencyInsane,

    kNumLatencySelections
};

struct BandParameters
{
    int* enabled = nullptr;
    int* type = nullptr;
    int* routing = nullptr;
    int* freq = nullptr;
    int* gain = nullptr;
    int* Q = nullptr;
    int* cutOrder = nullptr;
};

namespace FilterTypeNames
{
static const char* typeNames[kNumFilterTypes] = { "Peak", "Low Shelf", "High Shelf", "Low Cut", "High Cut" };
}

namespace RoutingTypeNames
{
static const char* routingNames[kNumRoutings] = { "Stereo", "Left", "Right", "Mid", "Side" };
}

namespace CutOrderTypeNames
{
static const char* cutOrderNames[kNumCutOrders] = { "12 dB/Oct", "24 dB/Oct", "48 dB/Oct" };
}
