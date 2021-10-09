#pragma once
#include "esphome.h"
#include <bitset>

enum FanMode { FAN_LOW, FAN_MEDIUM, FAN_HIGH, FAN_AUTO };

class ElectraDamperRemoteAction {    
    private:
    	const int ZERO_VAL = 540;
    	const int ONE_VAL = 1220;
    	const int START_VAL = 4000;
    	const int END_VAL = 8000;
    	const float TOLERANCE = 0.4;
    
    	int channelId = 0;
    	bool turnOn = true;
    	int temperature = 22;
    	FanMode fanMode = FAN_AUTO;	
    	int extra = 0b111;
    	bool valid = true;

	public:
		ElectraDamperRemoteAction() {
		}
		ElectraDamperRemoteAction(int channelId, bool turnOn, int temperature, FanMode fanMode) {
				this->channelId = channelId;
				this->turnOn = turnOn;
				this->temperature = temperature;
				this->fanMode = fanMode;
				this->extra = 0b111;
				valid = true;
		}
		
		bool isLike(ElectraDamperRemoteAction &other) {
			return this->turnOn == other.turnOn &&
			       this->temperature == other.temperature &&
				   this->fanMode == other.fanMode;
		}
		
		ElectraDamperRemoteAction(std::vector<int> codes) {
          static char binValue[17];
          char c = '*';
          int charIndex = 0;
          int valIndex = 0;
          bool valid = false;
          
          int val = codes[valIndex];
          
          // check if sequence starts with START_VAL and length of sequence is suitable for AC Damper Remote 
          if (val > START_VAL * (1 - TOLERANCE)  and abs(val) < START_VAL * (1 + TOLERANCE) && codes.size() == 104) 
          {
              valIndex+=2;
              val = codes[valIndex];
              
              while (abs(val) < END_VAL * (1 - TOLERANCE) && valIndex < codes.size() && !valid) {
                  if (val < 0) {
                    if (abs(val) > START_VAL * (1 - TOLERANCE)  and abs(val) < START_VAL * (1 + TOLERANCE)) {
                          // check parity bit
                          char parity = '0';
                          for(int i=0; i<charIndex-1; i++) {
                              if (binValue[i] == '1') {
                                  if (parity == '0') {
                                    parity = '1';
                                  } else {
                                    parity = '0';
                                  }
                              }
                          }
                          if (parity == binValue[charIndex-1]) {
                              valid = true;
                          }
                          // reset char position and starts again (if not valid) 
                          charIndex = 0;
                          c = ' ';
                      } else {
                          if (abs(val) > ZERO_VAL * (1 - TOLERANCE)  and abs(val) < ZERO_VAL * (1 + TOLERANCE)) {
                              c = '0';
                          } else if (abs(val) > ONE_VAL * (1 - TOLERANCE)  and abs(val) < ONE_VAL * (1 + TOLERANCE)) {
                              c = '1';
                          } else {
                              c = 'E';
                          }
                          binValue[charIndex] = c;
                          charIndex++;
                      }
                  }
                  valIndex++;
                  val = codes[valIndex];
              }
              binValue[16]='\0';
              if (!valid) {
                  // check parity bit of the last try
                  char parity = '0';
                  for(int i=0; i<charIndex-1; i++) {
                      if (binValue[i] == '1') {
                          if (parity == '0') {
                            parity = '1';
                          } else {
                            parity = '0';
                          }
                      }
                  }
                  if (parity == binValue[charIndex-1]) {
                      valid = true;
                  }
              }
              if (valid) {
                  this->valid = true;
                  //ESP_LOGD("ir", "%s", binValue);
                  // extract fan mode
                  if (binValue[6] == '0' && binValue[7] == '0') {
                      this->fanMode = FAN_LOW;
                  } else if (binValue[6] == '0' && binValue[7] == '1') {
                      this->fanMode = FAN_HIGH;
                  } else if (binValue[6] == '1' && binValue[7] == '0') {
                      this->fanMode = FAN_MEDIUM;
                  } else {
                      this->fanMode = FAN_AUTO;
                  }
                  
                  // extract ac state
                  if (binValue[14]=='1') {
                    turnOn = true;
                  } else {
                    turnOn = false;
                  }
                  
                  // extract temperature
                  float temp = 5;
                  if (binValue[8]=='1') temp+=1;
                  if (binValue[9]=='1') temp+=2;
                  if (binValue[10]=='1') temp+=4;
                  if (binValue[11]=='1') temp+=8;
                  if (binValue[12]=='1') temp+=16;
                  this->temperature = temp;
                  
                  // extract channelId 
                  int channelId = 0;
                  if (binValue[0]=='1') channelId+=1;
                  if (binValue[1]=='1') channelId+=2;
                  if (binValue[2]=='1') channelId+=4;
                  this->channelId = channelId;
                  
                  // extract extra value
                  int extra = 0;
                  if (binValue[0]=='1') extra+=1;
                  if (binValue[1]=='1') extra+=2;
                  if (binValue[2]=='1') extra+=4;
                  this->extra = extra;
                  
              }
              else {
                  this->valid = false;
				  // invalid sequence
                  //ESP_LOGD("ir", "invalid sequence");
              }
          } else {
              this->valid = false;
			  // unknown signal
              //ESP_LOGD("ir", "unknown signal");
          }
		}
		
		std::vector<int> getCodes() {
            bool parity = false;
            std::vector<int> codes;
            
            for(int j=0; j<3; j++) {
                codes.push_back(START_VAL);
                codes.push_back(-START_VAL);
                
                // channel id
                auto channelIdBits = std::bitset<3>(channelId);
                for(int i=0; i<3; i++) {
                    if (channelIdBits[i]==1) {
                        codes.push_back(ZERO_VAL);
                        codes.push_back(-ONE_VAL);
                        if (parity) { parity = false; } else {parity = true;};
                    } else {
                        codes.push_back(ZERO_VAL);
                        codes.push_back(-ZERO_VAL);
                    }
                }
                
                // extra bits
                auto extraBits = std::bitset<3>(extra);
                for(int i=0; i<3; i++) {
                    if (extraBits[i]==1) {
                        if (parity) { parity = false; } else {parity = true;};
                        codes.push_back(ZERO_VAL);
                        codes.push_back(-ONE_VAL);
                    } else {
                        codes.push_back(ZERO_VAL);
                        codes.push_back(-ZERO_VAL);
                    }
                }
                
                // fan mode
                switch(fanMode) {
                    case FAN_LOW:
                        codes.push_back(ZERO_VAL);
                        codes.push_back(-ZERO_VAL);
                        codes.push_back(ZERO_VAL);
                        codes.push_back(-ZERO_VAL);
                        break;
                    case FAN_HIGH:
                        codes.push_back(ZERO_VAL);
                        codes.push_back(-ZERO_VAL);
                        codes.push_back(ZERO_VAL);
                        codes.push_back(-ONE_VAL);
                        if (parity) { parity = false; } else {parity = true;};
                        break;
                    case FAN_MEDIUM:
                        codes.push_back(ZERO_VAL);
                        codes.push_back(-ONE_VAL);
                        codes.push_back(ZERO_VAL);
                        codes.push_back(-ZERO_VAL);
                        if (parity) { parity = false; } else {parity = true;};
                        break;
                    default:
                        codes.push_back(ZERO_VAL);
                        codes.push_back(-ONE_VAL);
                        codes.push_back(ZERO_VAL);
                        codes.push_back(-ONE_VAL);
                }
                
                // temp bits
                auto tempBits = std::bitset<6>(temperature-5);
                for(int i=0; i<6; i++) {
                    if (tempBits[i]==1) {
                        if (parity) { parity = false; } else {parity = true;};
                        codes.push_back(ZERO_VAL);
                        codes.push_back(-ONE_VAL);
                    } else {
                        codes.push_back(ZERO_VAL);
                        codes.push_back(-ZERO_VAL);
                    }
                }
                
                // on/off
                if (turnOn) {
                    codes.push_back(ZERO_VAL);
                    codes.push_back(-ONE_VAL);
                    if (parity) { parity = false; } else {parity = true;};
                } else {
                    codes.push_back(ZERO_VAL);
                    codes.push_back(-ZERO_VAL);
                }
                
                // parity bit
                if (parity) {
                    codes.push_back(ZERO_VAL);
                    codes.push_back(-ONE_VAL);
                } else {
                    codes.push_back(ZERO_VAL);
                    codes.push_back(-ZERO_VAL);
                }
            }
            
            codes.push_back(END_VAL);
			
            return codes;
        }

        int getTemperature() { return temperature; }
        void setTemperature(float temperature) { this->temperature = temperature;} 

        bool isOn() { return turnOn; }
        void setOn(bool isOn) { this->turnOn = isOn;}
		
        FanMode getFanMode() { return fanMode; }
        void setFanMode(FanMode fanMode) { this->fanMode = fanMode;}

        int getChannelId() { return channelId; }
        void setChannelId(int channelId) { this->channelId = channelId;}

		int getExtra() { return extra; }
        void setExtra(int extra) { this->extra = extra;}

        bool isValid() { return valid; }
};

class ElectraDamperClimate : public Component, public Climate {
	protected:
		remote_transmitter::RemoteTransmitterComponent *transmitter;
		ElectraDamperRemoteAction last_remote_state;
		ElectraDamperRemoteAction last_ac_full_state;

	public:
		void setup() override {
		}

		void on_receive(std::vector<int> codes) {
			auto action = ElectraDamperRemoteAction(codes);
			if (action.isValid()) {
				ESP_LOGD("ir", "on_receive: [%d] - %s %d° %d", action.getChannelId(), action.isOn()? "ON" : "OFF", (int)action.getTemperature(), action.getFanMode());
				// if the remote is configured to a channel other than 0 (default), make sure we won't retransmit repeated "turn on" actions
				if (action.getChannelId() != 0 && action.isLike(last_remote_state)) {
					// if so - use the already stored AC state instead of the remote state, to prevent remote from turning off the AC.
					// use only extra parameter from the remote.
					ESP_LOGD("ir", "received repeated remote action, changing it to preserve the last AC state.");
					action.setChannelId(last_ac_full_state.getChannelId());
					action.setTemperature(last_ac_full_state.getTemperature());
					action.setOn(last_ac_full_state.isOn());
					action.setFanMode(last_ac_full_state.getFanMode());
				} else {
					// otherwise - this is a new remote action, so we'll use it
					last_remote_state.setChannelId(action.getChannelId());
					last_remote_state.setTemperature(action.getTemperature());
					last_remote_state.setOn(action.isOn());
					last_remote_state.setFanMode(action.getFanMode());
					
					last_ac_full_state.setChannelId(action.getChannelId());
					last_ac_full_state.setTemperature(action.getTemperature());
					last_ac_full_state.setOn(action.isOn());
					last_ac_full_state.setFanMode(action.getFanMode());
				}

				auto call = id(ac_climate).make_call();
				// set fan mode
				switch(action.getFanMode()) {
				   case FanMode::FAN_LOW :
					  call.set_fan_mode("LOW");
					  id(ac_fanmode).publish_state("low");
					  break; 
				   case FanMode::FAN_MEDIUM :
					  call.set_fan_mode("MEDIUM");
					  id(ac_fanmode).publish_state("medium");
					  break; 
				   case FanMode::FAN_HIGH :
					  call.set_fan_mode("HIGH");
					  id(ac_fanmode).publish_state("high");
					  break; 
				   default : 
					  call.set_fan_mode("AUTO");
					  id(ac_fanmode).publish_state("auto");
				}

				// extract ac state
				if (action.isOn()) {
				  id(ac_state).publish_state("on");
				  call.set_mode("AUTO");
				} else {
				  id(ac_state).publish_state("off");
				  call.set_mode("OFF");
				}

				// extract temperature
				float temp = action.getTemperature();
				call.set_target_temperature(temp);
				id(ac_target_temperature).publish_state(temp);
				call.perform();

				// re-transmit the updated state
				id(last_ac_temperature) = temp;
				id(last_ac_state) = action.isOn();
				id(last_ac_fanmode) = (int) action.getFanMode();
			} else {
				ESP_LOGD("ir", "on_receive: unknown sequence, starting with %d", codes[0]);
			}
		}
		  
		void control(const ClimateCall &call) override {
			if (call.get_mode().has_value()) {
			  // User requested mode change
			  ClimateMode mode = *call.get_mode();
			  // Publish updated state
			  this->mode = mode;
			  this->publish_state();
			}
			if (call.get_target_temperature().has_value()) {
			  // User requested target temperature change
			  float temp = *call.get_target_temperature();
			  // Send target temp to climate
			  this->target_temperature = temp;
			  this->publish_state();
			}
			if (call.get_fan_mode().has_value()) {
			  // User requested target temperature change
			  auto fan_mode = *call.get_fan_mode();
			  // Send fan mode to climate
			  this->fan_mode = fan_mode;
			  this->publish_state();	
			}

			// transmit new state to the AC
			FanMode fanMode = FanMode::FAN_AUTO;
			if (this->fan_mode == CLIMATE_FAN_LOW) 			fanMode = FanMode::FAN_LOW;
			else if (this->fan_mode == CLIMATE_FAN_MEDIUM) 	fanMode = FanMode::FAN_MEDIUM;
			else if (this->fan_mode == CLIMATE_FAN_HIGH)    fanMode = FanMode::FAN_HIGH;
			else fanMode = FanMode::FAN_AUTO;

			auto action = ElectraDamperRemoteAction(0,  this->mode != CLIMATE_MODE_OFF, (int)this->target_temperature, fanMode);
			
			// update AC last state
			last_ac_full_state.setChannelId(action.getChannelId());
			last_ac_full_state.setTemperature(action.getTemperature());
			last_ac_full_state.setOn(action.isOn());
			last_ac_full_state.setFanMode(action.getFanMode());
			
			auto transmit = this->transmitter->transmit();
			transmit.get_data()->set_data(action.getCodes());
			transmit.get_data()->set_carrier_frequency(38000);
			transmit.perform();			
		}

		ClimateTraits traits() override {
		// The capabilities of the climate device
		auto traits = climate::ClimateTraits();
		traits.set_supports_current_temperature(true);
		traits.add_supported_mode(CLIMATE_MODE_OFF);
		traits.add_supported_mode(CLIMATE_MODE_AUTO);
		traits.add_supported_fan_mode(CLIMATE_FAN_LOW);
		traits.add_supported_fan_mode(CLIMATE_FAN_MEDIUM);
		traits.add_supported_fan_mode(CLIMATE_FAN_HIGH);
		traits.add_supported_fan_mode(CLIMATE_FAN_AUTO);
		return traits;
		}

		void set_current_temperature(float temp) {
		  ESP_LOGD("ir", "set_current_temperature to %f", temp);
		  this->current_temperature = temp;
		  this->publish_state();
		}

		void set_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) {
		 this->transmitter = transmitter;
		}
};
