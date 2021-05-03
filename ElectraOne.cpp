/*
   Copyright (c) 2021 Christof Ruch. All rights reserved.

   Dual licensed: Distributed under Affero GPL license by default, an MIT license is available for purchase
*/

#include "ElectraOne.h"

#include "Capability.h"
#include "MidiLocationCapability.h"

#include "BinaryResources.h"

#include <nlohmann/json-schema.hpp>

namespace midikraft {

	std::map<ElectraOneColor, std::string> kElectraOneColorValues = {
		{ElectraOneColor::white, "FFFFFF"},
		{ElectraOneColor::red, "F45C51 "},
		{ElectraOneColor::orange, "F49500"},
		{ElectraOneColor::blue, "529DEC"},
		{ElectraOneColor::green, "03A598"},
		{ElectraOneColor::pink, "C44795"},
	};

	std::string ElectraOneParameter::name() const
	{
		return name_;
	}

	ElectraOneControlType ElectraOneParameter::controlType() const
	{
		return controlType_;
	}

	ElectraOneInstrumentDefinition::ElectraOneInstrumentDefinition() 
	{
	}

	void ElectraOneInstrumentDefinition::addControllers(std::vector<std::shared_ptr<ElectraOneParameter>> const &controllers)
	{
		controllers_ = controllers;
	}

	std::string ElectraOneInstrumentDefinition::createJson(std::shared_ptr<Synth> synth)
	{
		// Check if the synth has a channel we need to create the value change messages later
		MidiChannel synthChannel = MidiChannel::fromZeroBase(0);
		auto location = Capability::hasCapability<MidiLocationCapability>(synth);
		if (location) {
			synthChannel = location->channel();
		}

		// Start creating the JSON data
		nlohmann::ordered_json instrument_definition;

		instrument_definition["id"] = "testid";
		instrument_definition["name"] = "Korg DW8000";
		instrument_definition["manufacturer"] = "Korg";
		instrument_definition["manufacturerId"] = "Korg";

		instrument_definition["categories"] = nlohmann::json::array({ { "id", "global" }, { "label", "Globals" } });

		instrument_definition["overlays"] = nlohmann::json::array();
		int overlayId = 1;
		std::map<std::shared_ptr<SynthParameterDefinition>, int> overlaysCreated;
		for (auto const &controller : controllers_) {
			if (controller->param()->type() == SynthParameterDefinition::ParamType::LOOKUP || controller->param()->type() == SynthParameterDefinition::ParamType::LOOKUP_ARRAY) {
				// It's a lookup parameter - create an "overlay" in Electra One terms
				nlohmann::ordered_json overlay;
				overlay["id"] = overlayId++;
				overlay["name"] = controller->param()->name();
				auto intParam = std::dynamic_pointer_cast<SynthIntValueParameterCapability>(controller->param());
				auto lookupParam = std::dynamic_pointer_cast<SynthLookupParameterCapability>(controller->param());
				if (intParam && lookupParam) {
					for (int value = 0; value <= intParam->maxValue(); value++) {
						overlay["items"].push_back({ {"value", value}, { "label",  lookupParam->valueAsText(value) } });
					}
					instrument_definition["overlays"].push_back(overlay);
					overlaysCreated[controller->param()] = overlay["id"];
				}
				else {
					SimpleLogger::instance()->postMessage("Need both SynthIntValueParameterCapability and SynthLookupParameterCapability to create overlay values");
				}
			}
		}

		instrument_definition["parameters"] = nlohmann::json::array();
		
		for (auto const &controller : controllers_) {
			nlohmann::ordered_json parameter;
			parameter["categoryId"] = "global";
			parameter["type"] = "fader";
			parameter["name"] = controller->name();
			parameter["values"] = ElectraOnePreset::createValues(controller->param(), synthChannel, overlaysCreated);
			instrument_definition["parameters"].push_back(parameter);
		}

		// Now, validate that we produced a valid Electra instrument definition file
		nlohmann::json_schema::json_validator validator;
		bool canValidate = true;
		try {
			std::string eif_schema(instr_json, instr_json + instr_json_size);
			validator.set_root_schema(nlohmann::json::parse(eif_schema));
		}
		catch (const std::exception &e) {
			SimpleLogger::instance()->postMessage("Validation of schema failed: " + String(e.what()));
			canValidate = false;
		}

		if (canValidate) {
			try {
				validator.validate(instrument_definition);
			}
			catch (const std::exception &e) {
				SimpleLogger::instance()->postMessage("Program error, invalid Electra Instrument Definition json created: " + String(e.what()));
				return "";
			}
		}

		return instrument_definition.dump();
	}

	ElectraOnePreset::ElectraOnePreset(std::string const &name, std::string const &id) : name_(name), id_(id)
	{
	}

	void ElectraOnePreset::addControllers(std::vector<std::shared_ptr<ElectraOneParameter>>const &controllers)
	{
		controllers_ = controllers;
	}

	std::vector<std::shared_ptr<midikraft::ElectraOneParameter>> ElectraOnePreset::controllers()
	{
		return controllers_;
	}

	std::string ElectraOnePreset::createJson(MidiChannel channel)
	{
		nlohmann::ordered_json preset;
		preset["version"] = 2;
		preset["name"] = name_;
		preset["projectId"] = id_;
		preset["pages"] = nlohmann::json::array();
		preset["devices"] = nlohmann::json::array({ { "id", 1 }, { "name", name_ },  { "instrumentID", id_ }, { "port", 1 }, { "channel", channel.toOneBasedInt() } });
		preset["overlays"] = nlohmann::json::array();
		preset["groups"] = nlohmann::json::array();
		preset["controls"] = nlohmann::json::array();
		createControllers(preset);

		return preset.dump(4);
	}

	void ElectraOnePreset::createControllers(nlohmann::ordered_json &preset)
	{
		int xgrid = 170;
		int ygrid = 88;
		int controlID = 1;
		for (auto const &controller : controllers_) {
			nlohmann::ordered_json control;
			jassert(controlID >= 1 && controlID <= 432);
			control["id"] = controlID++;
			control["name"] = controller->name();
			control["color"] = kElectraOneColorValues[controller->color()];
			control["pageId"] = 1;
			control["controlSetId"] = 1;
			control["type"] = "fader";
			control["bounds"] = { 0 * xgrid, 0 * ygrid, 146, 56 };
			control["values"] = nlohmann::ordered_json::array();

			// Create the MidiMessage that changes this value
			nlohmann::json controllerMsg;
			controllerMsg["deviceId"] = 1;

			auto values = createValues(controller->param(), MidiChannel::fromZeroBase(0), {});
			control["values"] = values;
			control["inputs"] = nlohmann::json::array();
			control["inputs"].push_back({ {"podId", controller->encoderNumber() }, { "valueId", values[0]["id"]} });

			// Add this to our list of controls
			preset["controls"].push_back(control);
		}
	}

	nlohmann::ordered_json ElectraOnePreset::createValues(std::shared_ptr<SynthParameterDefinition> param, MidiChannel channel, std::map<std::shared_ptr<SynthParameterDefinition>, int> overlaysCreated)
	{
		nlohmann::ordered_json result = nlohmann::ordered_json::array();

		auto intParam = std::dynamic_pointer_cast<SynthIntParameterCapability>(param);
		nlohmann::ordered_json value = {
			{ "id", param->name() },
			{"min", intParam->minValue() },
			{"max", intParam->maxValue() },
			{"defaultValue", 0}
		};
		auto lookupParam = std::dynamic_pointer_cast<SynthLookupParameterCapability>(param);
		if (lookupParam) {
			if (overlaysCreated.find(param) != overlaysCreated.end()) {
				value["overlayId"] = overlaysCreated[param];
			}
			else {
				//jassertfalse;
			}
		}
		nlohmann::ordered_json message = {
			{ "deviceId", 1},
			{ "type", "sysex" },
			{ "parameterNumber", intParam->sysexIndex() },
			{"min", intParam->minValue() },
			{"max", intParam->maxValue() },
			//TODO construction of this array needs to go into a new capability. And sysexIndex() is wrong, it is actually the parameter number
			{"data", { 0xf0, 0x42, 0x30 | channel.toZeroBasedInt(), 0x03, 0x41, intParam->sysexIndex(), { { "type", "value" }}, 0xf7}}
		};
		value["message"] = message;

		result.push_back(value);
		return result;
	}


}