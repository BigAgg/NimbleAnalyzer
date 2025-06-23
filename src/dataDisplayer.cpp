#include "dataDisplayer.h"
#include <imgui.h>
#include "utils.h"
#include "ui_helper.h"


void DisplayData(RowInfo &data, const unsigned int identifier, const std::string &mode, const std::vector<std::string> &hiddenHeaders) {
	// Drawing vertical with headers on right side
	if (mode == "vertical-rightheader") {
		int headerfix = 0;	// Needed for labels to not have multiple with same name
		for (auto& rdata : data.GetData()) {
			// Check if this value should be hidden and skip if it is in hiddenHeaders
			auto it = std::find(hiddenHeaders.begin(), hiddenHeaders.end(), rdata.first);
			if (it != hiddenHeaders.end()) {
				if (*it == rdata.first)
					continue;
			}
			// Generate strings to evaluate everything for InputStringWithHint
			std::string label = rdata.first + " ## " + std::to_string(identifier) + std::to_string(headerfix);
			std::string value = rdata.second;
			std::string headersplit = Splitlines(rdata.first, " ##").first;
			// Display the inputbox with hint
			ImGui::SetNextItemWidth(DEFAULT_INPUT_WIDTH);
			if(ImGui::InputStringWithHint(value, label, headersplit.c_str()))
				data.UpdateData(rdata.first, value);
			ImGui::SetItemTooltip(headersplit.c_str());
			headerfix++;
		}
		ImGui::Separator();
	}
	// Drawing vertical with headers on left side
	else if (mode == "vertical-leftheader") {
		int headerfix = 0;	// Needed to avoid multiple same labels
		for (auto& rdata : data.GetData()) {
			// Check if this one should be skipped
			auto it = std::find(hiddenHeaders.begin(), hiddenHeaders.end(), rdata.first);
			if (it != hiddenHeaders.end()) {
				if (*it == rdata.first)
					continue;
			}
			// Generate string to evaluate everything for INputStringWithHint
			std::string headersplit = Splitlines(rdata.first, " ##").first;
			std::string label = "## " + headersplit + std::to_string(identifier) + std::to_string(headerfix);
			std::string value = rdata.second;
			// Display the data and inputbox
			ImGui::Text("%s", headersplit.c_str());
			ImGui::SameLine();
			ImGui::SetNextItemWidth(DEFAULT_INPUT_WIDTH);
			label = "## " + label;
			if (ImGui::InputStringWithHint(value, label, headersplit.c_str()))
				data.UpdateData(rdata.first, value);
			ImGui::SetItemTooltip(headersplit.c_str());
			headerfix++;
		}
		ImGui::Separator();
	}
	// Drawing horizontal with headers above
	else if (mode == "horizontal-aboveheader") {
		auto&& dataset = data.GetData();
		int headerfix = 0;	// Needed to avoid multiple labels with same name
		for (auto& rdata : dataset) {
			// Check if this one should be skipped
			auto it = std::find(hiddenHeaders.begin(), hiddenHeaders.end(), rdata.first);
			if (it != hiddenHeaders.end()) {
				continue;
			}
			if (rdata != dataset.front() && headerfix > 0)
				ImGui::SameLine();
			// Generate strings to evaluate everything for InputStringWithHint
			std::string headersplit = Splitlines(rdata.first, " ##").first;
			std::string label = "## " + headersplit + std::to_string(identifier) + std::to_string(headerfix);
			std::string childname = label + "_child" + std::to_string(identifier);
			std::string value = rdata.second;
			// Begin a new childwindow (to be able to put them all side by side)
			// Also draw the data and inputbox
			ImGui::BeginChild(childname.c_str(), {DEFAULT_INPUT_WIDTH, 50.0f});
			ImGui::Text("%s", headersplit.c_str());
			ImGui::SetNextItemWidth(DEFAULT_INPUT_WIDTH);
			if (ImGui::InputStringWithHint(value, label, headersplit.c_str())) {
				data.UpdateData(rdata.first, value);
			}
			ImGui::SetItemTooltip(headersplit.c_str());
			ImGui::EndChild();
			headerfix++;
		}
		ImGui::Separator();
	}
	// Drawing horizontal without headers (Should be drawn before this)
	else if (mode == "horizontal-noheader") {
		auto&& dataset = data.GetData();
		int headerfix = 0;	// Needed to avoid multiple labels with same value
		for (auto& rdata : dataset) {
			// Check if this one should be skipped
			auto it = std::find(hiddenHeaders.begin(), hiddenHeaders.end(), rdata.first);
			if (it != hiddenHeaders.end()) {
				continue;
			}
			if (rdata != dataset.front() && headerfix > 0)
				ImGui::SameLine();
			// Generate strings to evaluate everything for InputStringWithHint
			std::string headersplit = Splitlines(rdata.first, " ##").first;
			std::string label = "## " + headersplit + std::to_string(identifier) + std::to_string(headerfix);
			std::string childname = label + "_child" + std::to_string(identifier);
			std::string value = rdata.second;
			// Begin drawing all values
			ImGui::BeginChild(childname.c_str(), {DEFAULT_INPUT_WIDTH, 25.0f});
			ImGui::SetNextItemWidth(DEFAULT_INPUT_WIDTH);
			if (ImGui::InputStringWithHint(value, label, headersplit.c_str())) {
				data.UpdateData(rdata.first, value);
			}
			ImGui::SetItemTooltip(headersplit.c_str());
			ImGui::EndChild();
			headerfix++;
		}
		ImGui::Separator();
	}

}

void DisplayDataset(std::vector<RowInfo>& data, const std::string& mode, const std::vector<std::string>& hiddenHeaders) {
	int idx = 0;
	for (auto& rowinfo : data) {
		DisplayData(rowinfo, idx, mode, hiddenHeaders);
		idx++;
	}
}