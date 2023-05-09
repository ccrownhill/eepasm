#include <cstdint>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <algorithm> // for transform

#include "eepasm.h"

// needs to have std::istream and not ifstream to work for string streams too
std::string get_low_str(std::istream& infile) {
	std::string out;
	infile >> out;
	std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
	return out;
}

std::unordered_map<std::string, std::string> reg_opgen(std::ifstream& cfile) {
	std::unordered_map<std::string, std::string>  opfield_map;

	opfield_map["lsb"] = get_cfile_val(cfile, "lsb");

	return opfield_map;
}

std::unordered_map<std::string, std::string> imm_opgen(std::ifstream& cfile) {
	std::unordered_map<std::string, std::string>  opfield_map;

	opfield_map["size"] = get_cfile_val(cfile, "size");
	opfield_map["lsb"] = get_cfile_val(cfile, "lsb");
	opfield_map["ins8"] = get_cfile_val(cfile, "ins8");
	if (!(opfield_map["ins8"] == "0" || opfield_map["ins8"] == "1"))
		throw parsing_error {"ins8 value must be 0 or 1"};

	return opfield_map;
}

std::unordered_map<std::string, std::string> lit_opgen(std::ifstream& cfile) {
	std::unordered_map<std::string, std::string>  opfield_map;

	opfield_map["name"] = get_cfile_val(cfile, "name");
	opfield_map["const"] = get_cfile_val(cfile, "const");

	return opfield_map;
}

std::unordered_map<std::string, std::string> no_opgen(std::ifstream& cfile) {
	std::unordered_map<std::string, std::string>  opfield_map {};

	// empty opfield map

	return opfield_map;
}

std::string get_cfile_val(std::ifstream& cfile, const std::string& field_name) {
	std::string instr = get_low_str(cfile);
	if (instr != field_name)
		throw parsing_error {"'" + field_name + "' field missing"};
	instr = get_low_str(cfile);
	return instr;
}

bool reg_check(const std::string& op, std::unordered_map<std::string, std::string>& opmap) {
	return op[0] == 'r';
}

bool imm_check(const std::string& op, std::unordered_map<std::string, std::string>& opmap) {
	return ((op[0] >= '0' && op[0] <= '9') || op[0] == '-');
}

bool label_check(const std::string& op, std::unordered_map<std::string, std::string>& opmap) {
	return true;
}

bool lit_check(const std::string& op, std::unordered_map<std::string, std::string>& opmap) {
	return (op == opmap["name"]);
}

void line_strip(std::string& line) {
	int comment_start = line.find("//");
	if (comment_start != -1)
		line = line.replace(comment_start, line.size()-1, "");

	while (line[0] == '\t' || line[0] == ' ')
		line = line.replace(0, 1, "");
	while (line[line.size()-1] == '\t' || line[line.size()-1] == ' ')
		line.resize(line.size()-1);
}

std::string ins2str(int pc, uint16_t iword) {
	std::ostringstream out;
	if (pc <= 0xf)
		out << "0x0" << std::hex << pc << " ";
	else
		out << "0x" << std::hex << pc << " ";

	if (iword <= 0xf)
		out << "0x000" << std::hex << iword;
	else if (iword <= 0xff)
		out << "0x00" << std::hex << iword;
	else if (iword <= 0xfff)
		out << "0x0" << std::hex << iword;
	else
		out << "0x" << std::hex << iword;

	return out.str();
}

uint16_t num_parse(const std::string& instr) {
	uint16_t num;
	if (instr.substr(0,2) == "0x")
		num = std::stoi(instr.substr(2), nullptr, 16); // base 16
	else if (instr.substr(0,2) == "0b")
		num = std::stoi(instr.substr(2), nullptr, 2); // base 2
	else
		num = std::stoi(instr); // base 10
	return num;
}

uint16_t reg_parse(const std::string& reg_name, std::unordered_map<std::string, std::string>& opfield_map, int pc) {
	return (reg_name[1] - '0') << num_parse(opfield_map["lsb"]);
}

uint16_t imm_parse(const std::string& imm_op, std::unordered_map<std::string, std::string>& opfield_map, int pc) {
	uint16_t bitmask = (1 << num_parse(opfield_map["size"])) - 1;
	uint16_t num = num_parse(imm_op);
	num = (num & bitmask) << num_parse(opfield_map["lsb"]);
	num += (num_parse(opfield_map["ins8"]) << 8);
	return num;
}

uint16_t label_parse(const std::string& label, std::unordered_map<std::string, std::string>& opfield_map, int pc) {
	if (label_map.find(label) == label_map.end())
		throw assem_error {"label '" + label + "' not found in program"};
	return (label_map[label] - static_cast<uint16_t>(pc)) & 0xff;
}

uint16_t lit_parse(const std::string& label, std::unordered_map<std::string, std::string>& opfield_map, int pc) {
	return num_parse(opfield_map["const"]);
}
