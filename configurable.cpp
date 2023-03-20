#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdint>
#include <unordered_map>
#include <functional> // for function objects
#include <vector>
#include <utility> // for pair

// TODO can you use using in header files
using oplist_t = std::vector<std::unordered_map<std::string, std::string>>;
using insmap_t = std::unordered_map<std::string, std::pair<std::vector<oplist_t>, uint16_t>>;
using labelmap_t = std::unordered_map<std::string, int>;
using tokvec_t = std::vector<std::vector<std::string>>;

constexpr char insfile[] = "inslist.eepc";
constexpr int regsize = 3;

std::pair<int, int> test;

void error(const std::string& msg);
void line_strip(std::string& line);
std::pair<tokvec_t, labelmap_t> tokenize_file(std::ifstream& infile, const insmap_t& insmap);
std::string ins2str(int pc, uint16_t iword);

bool optype_equal(const std::string& op, const std::string& type);

uint16_t num_parse(const std::string& instr);

uint16_t reg_parse(const std::string& reg_name, int size, int pc);
uint16_t imm_parse(const std::string& imm_op, int size, int pc);
uint16_t label_parse(const std::string& label, int size, int pc);
uint16_t no_parse(const std::string& op, int size, int pc);


insmap_t insmap_gen(const std::string& conf_file);
oplist_t opvec_gen(std::istream& cfile, int numops);

std::unordered_map<std::string, std::function<uint16_t(const std::string&, int, int)>> optype_fns {
	{"reg", reg_parse},
	{"imm", imm_parse},
	{"label", label_parse},
	{"PCX", no_parse},
	{"Flags", no_parse},
};

std::unordered_map<std::string, int> label_map;

int main(int argc, char *argv[]) {
	if (argc != 3)
		error("Usage: eepasm infile outfile");

	insmap_t insmap = insmap_gen(insfile);

	std::ifstream infile {argv[1]};
	if (!infile)
		error("can't open input file");

	auto [tok_vec, label_map] = tokenize_file(infile, insmap);
	infile.close();

	std::ofstream outfile {argv[2]};
	if (!outfile)
		error("can't open output file");


	int pc = 0, iword;
	std::vector<oplist_t> ins_alts;
	for (const auto& tokens : tok_vec) {
		ins_alts = insmap[tokens[0]].first;
		iword = insmap[tokens[0]].second;
		bool alt_found = false;

		int imm_size;
		int alt_idx = 0;
		// skip to first instruction alternative with correct number
		// of operands
		while (ins_alts[alt_idx].size() != tokens.size() - 1) {
			alt_idx++;
			if (alt_idx >= ins_alts.size())
				error("assembly: no matching verions of instruction found");
		}
		// select first alternative with matching number of operands
		if (ins_alts[alt_idx].size() == tokens.size() - 1) {
			alt_found = true;
			for (int j = 0; j < ins_alts[alt_idx].size(); j++) {
				// go to next alternative if current operand does not match requirement
				while (!optype_equal(tokens[1+j], ins_alts[alt_idx][j]["type"])) {
					alt_idx++;
					if (alt_idx >= ins_alts.size() || ins_alts[alt_idx].size() != tokens.size() - 1)
						error("assembly: no matching version of instruction found");

				}

				if (ins_alts[alt_idx][j]["type"] == "reg") {
					iword += optype_fns[ins_alts[alt_idx][j]["type"]](tokens[1+j], regsize, pc) << num_parse(ins_alts[alt_idx][j]["lsb"]);
				} else {
					imm_size = num_parse(ins_alts[alt_idx][j]["size"]);
					iword += optype_fns[ins_alts[alt_idx][j]["type"]](tokens[1+j], imm_size, pc) << num_parse(ins_alts[alt_idx][j]["lsb"]);
					if (imm_size == 8) { // need to set bit 8
						iword += 1 << 8;
					}
				}
			}
		}
		if (!alt_found)
			error("assembly: no matching version of instruction found");

		outfile << ins2str(pc, iword) << "\n";
		pc++;
	}
 	outfile.close();
}

void error(const std::string& msg) {
	std::cerr << "Error: " << msg << std::endl;
	std::exit(EXIT_FAILURE);
}


insmap_t insmap_gen(const std::string& conf_file) {
	std::ifstream cfile {conf_file};
	if (!cfile)
		error("Can't open instruction list config file");

	insmap_t outmap;
	std::vector<oplist_t> alternatives_vec;
	std::string ins_name, instr;
	int numops;
	uint16_t const_iword;
	while (cfile >> ins_name) {
		// process instructions stored in instr
		alternatives_vec.clear();
		
		while (cfile >> instr && instr == "numops") {
			cfile >> numops; // numops value
			alternatives_vec.push_back(opvec_gen(cfile, numops));
		}
		outmap[ins_name].first = alternatives_vec;

		// while already read string const_iword
		if (instr != "const_iword")
			error("parsing: missing const_iword field");
		cfile >> instr; // string of const_iword
		outmap[ins_name].second = num_parse(instr);
	}

	return outmap;
}

oplist_t opvec_gen(std::istream& cfile, int numops) {
	oplist_t outvec;
	std::unordered_map<std::string, std::string> opfield_map;
	std::string instr;
	for (int i = 0; i < numops; i++) {
		cfile >> instr;
		if (instr != "op")
			error("parsing: missing op indicator");

		cfile >> instr;
		if (instr != "type")
			error("parsing: type field missing");

		cfile >> instr; // set to type value
		if (optype_fns.find(instr) == optype_fns.end())
			error("parsing: invaid operand type");
		opfield_map["type"] = instr;
		
		if (instr != "reg") { // need size field
			cfile >> instr;
			if (instr != "size")
				error("parsing: size field missing");

			cfile >> instr; // set to size value
			opfield_map["size"] = instr;
		}

		cfile >> instr;
		if (instr != "lsb")
			error("parsing: lsb field missing");
		cfile >> instr; // set to lsb value
		opfield_map["lsb"] = instr;
		outvec.push_back(opfield_map);
	}
	return outvec;
}

std::pair<tokvec_t, labelmap_t> tokenize_file(std::ifstream& infile, const insmap_t& insmap) {
	tokvec_t outvec;
	labelmap_t labelmap;

	std::string line, token;
	std::istringstream line_stream;
	std::vector<std::string> token_vec;
	int pc = 0;
	while (getline(infile, line)) {
		if (line == "")
			continue;
		line_strip(line);
		line_stream.clear(); // clear state flags
		line_stream.str(line);
		token_vec.clear();

		line_stream >> token;
		if (insmap.find(token) == insmap.end()) {
			label_map[token] = pc;
			if (line_stream.str() == token) // if label on separate line
				continue; // need to skip incrementingpc
		} else {
			token_vec.push_back(token);
		}
		pc++;
			
		while (line_stream >> token) {
			while (token[0] == '#' || token [0] == '[')
				token = token.replace(0, 1, "");
			if (token[token.size()-1] == ',' || token[token.size()-1] == ']')
				token = token.replace(token.size()-1, token.size(), "");
			token_vec.push_back(token);
		}
		outvec.push_back(token_vec);
	}
	return make_pair(outvec, labelmap);
}

bool optype_equal(const std::string& op, const std::string& type) {
	if (type == "reg") {
		return (op[0] == 'R');
	} else if (type == "imm") {
		// last case to also make it work for negative numbers
		return ((op[0] > '0' && op[0] < '9') || op[0] == '-');
	} else if (type == "label") {
		return true;
	} else if (type == "PCX") {
		return (op == "PCX");
	} else if (type == "Flags") {
		return (op == "Flags");
	} else {
		return false; // if unknown type
	}
}

void line_strip(std::string& line) {
	int comment_start = line.find("//");
	if (comment_start != -1)
		line = line.replace(comment_start, line.size()-1, "");

	while (line[0] == '\t' || line[0] == ' ')
		line = line.replace(0, 1, "");
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

uint16_t reg_parse(const std::string& reg_name, int size, int pc) {
	return reg_name[1] - '0';
}

uint16_t imm_parse(const std::string& imm_op, int size, int pc) {
	uint16_t bitmask = (1 << size) - 1;
	uint16_t num = num_parse(imm_op);
	return num & bitmask;
}

uint16_t label_parse(const std::string& label, int size, int pc) {
	if (label_map.find(label) == label_map.end())
		error("assembly: label not found in program");
	return (label_map[label] - static_cast<uint16_t>(pc)) & 0xff;
}

uint16_t no_parse(const std::string& instr, int size, int pc) {
	return 0;
}
