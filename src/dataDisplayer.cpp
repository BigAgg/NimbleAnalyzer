#include "dataDisplayer.h"
#include <imgui.h>
#include "ui_helper.h"

#define DEFAULT_INPUT_WIDTH 175.0f

void DisplayData(RowInfo &data, const unsigned int identifier, const std::string &mode) {
	// Drawing vertical with headers on right side
	if (mode == "vertical-rightheader") {
		for (auto& rdata : data.GetData()) {
			std::string label = rdata.first + " ## " + std::to_string(identifier);
			std::string value = rdata.second;
			ImGui::SetNextItemWidth(DEFAULT_INPUT_WIDTH);
			if(ImGui::InputStringWithHint(value, label, rdata.first.c_str()))
				data.UpdateData(rdata.first, value);
		}
		ImGui::Separator();
	}
	// Drawing vertical with headers on left side
	else if (mode == "vertical-leftheader") {
		for (auto& rdata : data.GetData()) {
			std::string label = "## " + rdata.first + std::to_string(identifier);
			std::string value = rdata.second;
			ImGui::Text("%s", rdata.first.c_str());
			ImGui::SameLine();
			ImGui::SetNextItemWidth(DEFAULT_INPUT_WIDTH);
			label = "## " + label;
			if (ImGui::InputStringWithHint(value, label, rdata.first.c_str()))
				data.UpdateData(rdata.first, value);
		}
		ImGui::Separator();
	}
	// Drawing horizontal with headers above
	else if (mode == "horizontal-aboveheader") {
		auto&& dataset = data.GetData();
		//ImGui::BeginChild("horizontal-aboveheader-dataDisplayer", { dataset.size() * DEFAULT_INPUT_WIDTH + DEFAULT_INPUT_WIDTH, dataset.size() * 50.0f });
		for (auto& rdata : dataset) {
			std::string label = "## " + rdata.first + std::to_string(identifier);
			std::string childname = label + "_child";
			std::string value = rdata.second;
			ImGui::BeginChild(childname.c_str(), {DEFAULT_INPUT_WIDTH, 50.0f});
			ImGui::Text("%s", rdata.first.c_str());
			ImGui::SetNextItemWidth(DEFAULT_INPUT_WIDTH);
			if (ImGui::InputStringWithHint(value, label, rdata.first.c_str())) {
				data.UpdateData(rdata.first, value);
			}
			ImGui::EndChild();
			if (rdata != dataset.back())
				ImGui::SameLine();
		}
		ImGui::Separator();
		//ImGui::EndChild();
	}
}