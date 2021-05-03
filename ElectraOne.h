/*
   Copyright (c) 2021 Christof Ruch. All rights reserved.

   Dual licensed: Distributed under Affero GPL license by default, an MIT license is available for purchase
*/

#pragma once

#include "JuceHeader.h"

#include "Synth.h"
#include "SynthParameterDefinition.h"

#undef snprintf
#include "nlohmann/json.hpp"

namespace midikraft {

	// These are hard coded
	enum class ElectraOneColor {
		white,
		red,
		orange,
		blue,
		green,
		pink
	};

	enum ElectraOneControlType {
		ENCODER, // well, slider
		BUTTON,  // well, touch
	};

	class ElectraOneParameter {
	public:
		ElectraOneParameter(ElectraOneControlType controlType, std::string const name, int physicalEncoderNumber) : controlType_(controlType), name_(name), encoderNumber_(physicalEncoderNumber) {
			//jassert(physicalEncoderNumber >= 0 && physicalEncoderNumber <= 12);
		}

		std::string name() const;
		ElectraOneControlType controlType() const;;
		int encoderNumber() const { return encoderNumber_; }

		virtual ElectraOneColor color() const = 0;
		virtual std::shared_ptr<SynthParameterDefinition> param() const = 0;

	private:
		ElectraOneControlType controlType_;
		std::string name_;
		int encoderNumber_;
	};

	class ElectraOneInstrumentDefinition {
	public:
		ElectraOneInstrumentDefinition();

		void addControllers(std::vector<std::shared_ptr<ElectraOneParameter>> const &controllers);

		std::string createJson(std::shared_ptr<Synth> synth);

	private:
		std::shared_ptr<Synth> synth_;
		std::vector<std::shared_ptr<ElectraOneParameter>> controllers_;
	};

	class ElectraOnePreset {
	public:
		ElectraOnePreset(std::string const &name, std::string const &id);

		void addControllers(std::vector<std::shared_ptr<ElectraOneParameter>> const &controllers);
		std::vector<std::shared_ptr<ElectraOneParameter>> controllers();

		std::string createJson(MidiChannel channel);

		static nlohmann::ordered_json createValues(std::shared_ptr<SynthParameterDefinition> param, MidiChannel channel, std::map<std::shared_ptr<SynthParameterDefinition>, int> overlaysCreated);

	private:
		void createControllers(nlohmann::ordered_json &preset);

		std::string name_;
		std::string id_;
		std::vector<std::shared_ptr<ElectraOneParameter>> controllers_;
	};


}

