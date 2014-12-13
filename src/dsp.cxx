/*
 * Author: Harry van Haaren 2014
 *         harryhaaren@gmail.com
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "shared.hxx"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "dsp/ports.hxx"
#include "dsp/fabla2.hxx"
using namespace Fabla2;


LV2_Handle FablaLV2::instantiate( const LV2_Descriptor* descriptor,
                                  double samplerate,
                                  const char* bundle_path,
                                  const LV2_Feature* const* features)
{
  
  LV2_URID_Map* map = NULL;
  for (int i = 0; features[i]; ++i)
  {
    if (!strcmp(features[i]->URI, LV2_URID__map))
    {
      map = (LV2_URID_Map*)features[i]->data;
      break;
    }
  }
  if (!map)
    return 0;
  
  FablaLV2* tmp = new FablaLV2( samplerate );
  tmp->map = map;
  
  mapUri( &tmp->uris, map );
  
  lv2_atom_forge_init( &tmp->forge, map);
  
  return (LV2_Handle)tmp;
}

FablaLV2::FablaLV2(int rate)
{
  dsp = new Fabla2::Fabla2DSP( rate );
  if( !dsp )
  {
    printf("Fabla2DSP() failed in FablaLV2::instantiate() Aborting.\n");
  }
}

FablaLV2::~FablaLV2()
{
  delete dsp;
}

void FablaLV2::activate(LV2_Handle instance)
{
}

void FablaLV2::deactivate(LV2_Handle instance)
{
}

void FablaLV2::connect_port(LV2_Handle instance, uint32_t port, void *data)
{
  FablaLV2* self = (FablaLV2*) instance;
  
  switch (port)
  {
    // handle Atom ports gracefully here
      case ATOM_IN:
          self->control = (const LV2_Atom_Sequence*)data;
          break;
      
      case ATOM_OUT:
          self->notify  = (const LV2_Atom_Sequence*)data;
          break;
    
      default:
          self->dsp->controlPorts[port]     = (float*)data;
          break;
  }
}

void FablaLV2::run(LV2_Handle instance, uint32_t nframes)
{
  FablaLV2* self = (FablaLV2*) instance;
  
  // setup Forge for Atom output (to UI)
  const uint32_t notify_capacity = self->notify->atom.size;
  lv2_atom_forge_set_buffer(&self->forge, (uint8_t*)self->notify, notify_capacity);
  
  // Start a sequence in the notify output port.
  lv2_atom_forge_sequence_head(&self->forge, &self->notify_frame, 0);
  
  // handle incoming MIDI
  LV2_ATOM_SEQUENCE_FOREACH(self->control, ev)
  {
    if (ev->body.type == self->uris.midi_MidiEvent)
    {
      const uint8_t* const msg = (const uint8_t*)(ev + 1);
      self->dsp->midi( ev->time.frames, msg );
      
      if( msg[0] == 144 || msg[0] == 128 )
      {
        // write note on/off MIDI events to UI
        LV2_Atom_Forge_Frame frame;
        lv2_atom_forge_frame_time( &self->forge, ev->time.frames );
        lv2_atom_forge_object( &self->forge, &frame, 0, self->uris.fabla2_PadEvent );

        // Add UI state as properties
        lv2_atom_forge_key(&self->forge, self->uris.fabla2_pad);
        lv2_atom_forge_int(&self->forge, msg[1] );
        
        lv2_atom_forge_key(&self->forge, self->uris.fabla2_velocity);
        lv2_atom_forge_int(&self->forge, msg[2] );
        
      }
      
    }
    
  }
  
  self->dsp->process( nframes );
}

void FablaLV2::cleanup(LV2_Handle instance)
{
  delete ((FablaLV2*) instance);
}

const void* FablaLV2::extension_data(const char* uri)
{
  return NULL;
}



static const LV2_Descriptor descriptor = {
	FABLA2_URI,
	FablaLV2::instantiate,
	FablaLV2::connect_port,
	FablaLV2::activate,
	FablaLV2::run,
	FablaLV2::deactivate,
	FablaLV2::cleanup,
	FablaLV2::extension_data
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
  if( index == 0 )
    return &descriptor;
  
  return 0;
}
