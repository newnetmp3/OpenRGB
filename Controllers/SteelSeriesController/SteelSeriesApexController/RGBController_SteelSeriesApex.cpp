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


// Mini LED order follows the 68-entry HID feature report, not the generic
// 112-entry Apex list. Each value indexes the existing led_names table.
static const unsigned int mini_legacy_indices[] =
{
    37, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 41, 42, 91,
    39, 16, 22, 4, 17, 19, 24, 20, 8, 14, 15, 43, 44, 88,
    52, 0, 18, 3, 5, 6, 7, 9, 10, 11, 46, 47, 45, 36,
    80, 78, 25, 23, 2, 21, 1, 13, 12, 49, 50, 51, 89, 84,
    79, 82, 81, 93, 40, 92, 90, 85, 86, 87, 83, 38
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
    | Set up zones                                              |
    \*---------------------------------------------------------*/

    /*---------------------------------------------------------*\
    | The first 5 chars are the SKU which we need to determine  |
    | the region.                                               |
    \*---------------------------------------------------------*/

    std::string sku = serial.substr(0, 5);
    const SteelSeriesApexController* apex_controller = dynamic_cast<const SteelSeriesApexController*>(controller);
    const bool is_mini = apex_controller != nullptr && apex_controller->IsApexProMini();

    if(is_mini)
    {
        zone mini_zone;
        mini_zone.name = ZONE_EN_KEYBOARD;
        mini_zone.type = ZONE_TYPE_MATRIX;
        mini_zone.leds_min = sizeof(mini_legacy_indices) / sizeof(mini_legacy_indices[0]);
        mini_zone.leds_max = mini_zone.leds_min;
        mini_zone.leds_count = mini_zone.leds_min;
        mini_zone.matrix_map.height = MATRIX_HEIGHT;
        mini_zone.matrix_map.width = MATRIX_WIDTH;
        mini_zone.matrix_map.map.resize(MATRIX_HEIGHT * MATRIX_WIDTH);

        // The USB PID, not the SKU string, selects the Mini layout.
        // The observed Mini serial prefix (64865) is also used by a TKL.
        unsigned int legacy_matrix[MATRIX_HEIGHT][MATRIX_WIDTH] = MATRIX_MAP_ANSI;
        for(const matrix_region_patch& patch : apex_mini_us_region_patch)
        {
            legacy_matrix[patch.row][patch.column] = patch.value;
        }

        for(unsigned int cell = 0; cell < MATRIX_HEIGHT * MATRIX_WIDTH; ++cell)
        {
            mini_zone.matrix_map.map[cell] = NA;
            for(unsigned int led_idx = 0; led_idx < mini_zone.leds_count; ++led_idx)
            {
                if(legacy_matrix[cell / MATRIX_WIDTH][cell % MATRIX_WIDTH] == mini_legacy_indices[led_idx])
                {
                    mini_zone.matrix_map.map[cell] = led_idx;
                    break;
                }
            }
        }
        zones.push_back(mini_zone);
        for(unsigned int led_idx = 0; led_idx < mini_zone.leds_count; ++led_idx)
        {
            led new_led;
            new_led.name = led_names[mini_legacy_indices[led_idx]];
            leds.push_back(new_led);
        }
        SetupColors();
        return;
    }


    unsigned int total_led_count = 0;

    for(unsigned int zone_idx = 0; zone_idx < 1; zone_idx++)
    {
        zone new_zone;
        new_zone.name           = zone_names[zone_idx];
        new_zone.type           = zone_types[zone_idx];

        if(zone_types[zone_idx] == ZONE_TYPE_MATRIX)
        {
            new_zone.matrix_map.height      = MATRIX_HEIGHT;
            new_zone.matrix_map.width       = MATRIX_WIDTH;
            new_zone.matrix_map.map.resize(MATRIX_HEIGHT * MATRIX_WIDTH);

            if((proto_type == APEX) || (proto_type == APEX_M))
            {
                SetSkuRegion(&new_zone.matrix_map, sku);
            }
        }

        if((proto_type == APEX) || (proto_type == APEX_M))
        {
            new_zone.leds_min   = zone_sizes[zone_idx];
            new_zone.leds_max   = zone_sizes[zone_idx];
            new_zone.leds_count = zone_sizes[zone_idx];
            total_led_count    += zone_sizes[zone_idx];
        }
        zones.push_back(new_zone);
    };

    SetSkuLedNames(leds, sku, total_led_count);
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
