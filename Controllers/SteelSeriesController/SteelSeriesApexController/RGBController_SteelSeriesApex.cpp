/*---------------------------------------------------------*\
| RGBController_SteelSeriesApex.cpp                         |
|                                                           |
|   RGBController for SteelSeries Apex 7                    |
|                                                           |
|   Eric Samuelson (edbgon)                     05 Jul 2020 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "RGBControllerKeyNames.h"
#include "RGBController_SteelSeriesApex.h"
#include "SteelSeriesApexRegions.h"
#include "SteelSeriesApexController.h"


/*---------------------------------------------------------*\
| Apex Pro Mini lighting layout data                         |
\*---------------------------------------------------------*/
static const unsigned int mini_legacy_indices[] =
{
    37, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 41, 42, 91,
    39, 16, 22,  4, 17, 19, 24, 20,  8, 14, 15, 43, 44, 88,
    52,  0, 18,  3,  5,  6,  7,  9, 10, 11, 46, 47, 45, 36,
    80, 78, 25, 23,  2, 21,  1, 13, 12, 49, 50, 51, 89, 84,
    79, 82, 81, 93, 40, 92, 90, 85, 86, 87, 83, 38
};

/*---------------------------------------------------------*\
| US ANSI matrix indices into the 68-address HID report.   |
| Seven optional regional addresses are not visible keys.  |
\*---------------------------------------------------------*/
static const unsigned int mini_ansi_matrix[5][15] =
{
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 67, NA },
    { 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, NA },
    { 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 41, NA, NA },
    { 42, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 55, NA, NA, NA },
    { 56, 57, 58, 60, 63, 64, 65, 66, NA, NA, NA, NA, NA, NA, NA }
};

struct apex_keyboard_layout
{
    unsigned int        led_count;
    unsigned int        height;
    unsigned int        width;
    const unsigned int* matrix;
    const unsigned int* led_names;
};

static const apex_keyboard_layout mini_layout =
{
    sizeof(mini_legacy_indices) / sizeof(mini_legacy_indices[0]),
    5,
    15,
    &mini_ansi_matrix[0][0],
    mini_legacy_indices
};

static const char* zone_names[] =
{
    ZONE_EN_KEYBOARD
};

static zone_type zone_types[] =
{
    ZONE_TYPE_MATRIX
};

static const unsigned int zone_sizes[] =
{
    sizeof(led_names)/sizeof(char*),
};

/**------------------------------------------------------------------*\
    @name Steel Series APEX
    @category Keyboard
    @type USB
    @save :x:
    @direct :white_check_mark:
    @effects :x:
    @detectors DetectSteelSeriesApex,DetectSteelSeriesApexM
    @comment
\*-------------------------------------------------------------------*/

RGBController_SteelSeriesApex::RGBController_SteelSeriesApex(SteelSeriesApexBaseController* controller_ptr)
{
    controller          = controller_ptr;

    name                = controller->GetName();
    vendor              = "SteelSeries";
    type                = DEVICE_TYPE_KEYBOARD;
    description         = "SteelSeries Apex RGB Device";
    location            = controller->GetLocation();
    serial              = controller->GetSerial();
    version             = controller->GetVersion();

    proto_type          = controller->proto_type;

    mode Direct;
    Direct.name         = "Direct";
    Direct.value        = APEX_MODE_DIRECT;
    Direct.flags        = MODE_FLAG_HAS_PER_LED_COLOR;
    Direct.color_mode   = MODE_COLORS_PER_LED;
    modes.push_back(Direct);

    mode Onboard;
    Onboard.name        = "Onboard";
    Onboard.value       = APEX_MODE_ONBOARD;
    Onboard.flags       = 0;
    Onboard.color_mode  = MODE_COLORS_NONE;

    /*---------------------------------------------------------*\
    | The brightness setting applies to the on-board lighting.  |
    | Direct mode sends absolute colors and is not affected by  |
    | it, so it is offered on this mode only.                   |
    \*---------------------------------------------------------*/
    if(controller->SupportsBrightness())
    {
        Onboard.flags          |= MODE_FLAG_HAS_BRIGHTNESS;
        Onboard.brightness_min  = APEX_BRIGHTNESS_MIN;
        Onboard.brightness_max  = APEX_BRIGHTNESS_MAX;
        Onboard.brightness      = controller->GetBrightness();
    }

    modes.push_back(Onboard);

    SetupZones();
}

RGBController_SteelSeriesApex::~RGBController_SteelSeriesApex()
{
    Shutdown();

    delete controller;
}

void RGBController_SteelSeriesApex::SetupZones()
{
    /*---------------------------------------------------------*\
    | The first 5 chars are the SKU for the keyboard region.    |
    \*---------------------------------------------------------*/
    std::string sku = serial.substr(0, 5);

    const SteelSeriesApexController* apex_controller = dynamic_cast<const SteelSeriesApexController*>(controller);
    const apex_keyboard_layout* layout = nullptr;
    if(apex_controller != nullptr && apex_controller->IsApexProMini())
    {
        layout = &mini_layout;
    }

    unsigned int total_led_count = 0;

    for(unsigned int zone_idx = 0; zone_idx < 1; zone_idx++)
    {
        zone new_zone;
        new_zone.name           = zone_names[zone_idx];
        new_zone.type           = zone_types[zone_idx];

        if(zone_types[zone_idx] == ZONE_TYPE_MATRIX)
        {
            new_zone.matrix_map.height = layout != nullptr ? layout->height : MATRIX_HEIGHT;
            new_zone.matrix_map.width  = layout != nullptr ? layout->width : MATRIX_WIDTH;
            new_zone.matrix_map.map.resize(new_zone.matrix_map.height * new_zone.matrix_map.width);

            if(layout != nullptr)
            {
                for(unsigned int cell = 0; cell < layout->height * layout->width; cell++)
                {
                    new_zone.matrix_map.map[cell] = layout->matrix[cell];
                }
            }
            else if((proto_type == APEX) || (proto_type == APEX_M))
            {
                SetSkuRegion(&new_zone.matrix_map, sku);
            }
        }

        if((proto_type == APEX) || (proto_type == APEX_M))
        {
            unsigned int led_count = layout != nullptr ? layout->led_count : zone_sizes[zone_idx];
            new_zone.leds_min      = led_count;
            new_zone.leds_max      = led_count;
            new_zone.leds_count    = led_count;
            total_led_count      += led_count;
        }

        zones.push_back(new_zone);
    }

    if(layout != nullptr)
    {
        for(unsigned int led_idx = 0; led_idx < total_led_count; led_idx++)
        {
            led new_led;
            new_led.name = led_names[layout->led_names[led_idx]];
            leds.push_back(new_led);
        }
    }
    else
    {
        SetSkuLedNames(leds, sku, total_led_count);
    }

    SetupColors();
}

void RGBController_SteelSeriesApex::DeviceUpdateLEDs()
{
    last_update_time = std::chrono::steady_clock::now();
    controller->SetLEDsDirect(colors);
}

void RGBController_SteelSeriesApex::DeviceUpdateZoneLEDs(int /*zone*/)
{
    DeviceUpdateLEDs();
}

void RGBController_SteelSeriesApex::DeviceUpdateSingleLED(int /*led*/)
{
    DeviceUpdateLEDs();
}

void RGBController_SteelSeriesApex::DeviceUpdateMode()
{
    std::vector<RGBColor> temp_colors;

    if(modes[active_mode].flags & MODE_FLAG_HAS_BRIGHTNESS)
    {
        controller->SetBrightness((unsigned char)modes[active_mode].brightness);
    }

    controller->SetMode(modes[active_mode].value, temp_colors);
}
