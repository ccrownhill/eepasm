#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdint>
#include <unordered_map>
#include <functional> // for function objects
#include <vector>
#include <utility> // for pair
#include <algorithm> // for transform

using oplist_t = std::vector<std::unordered_map<std::string, std::string>>;
using insmap_t = std::unordered_map<std::string, std::pair<std::vector<oplist_t>, uint16_t>>;
using labelmap_t = std::unordered_map<std::string, int>;
using tokvec_t = std::vector<std::vector<std::string>>;

constexpr char def_insfile[] = "inslist.eepc";
constexpr char def_outfile[] = "out.ram";
constexpr int regsize = 3;
constexpr int offset_size = 8;

std::pair<int, int> test;


void usage();
void error(const std::string& msg);
void parsing_error(const std::string& msg, const std::string& insname);
void assem_error(const std::string& msg, const std::string& insname, int line);
std::string get_low_str(std::istream& infile);
std::string get_cfile_val(std::ifstream& cfile, const std::string& field_name);
void line_strip(std::string& line);
std::pair<tokvec_t, labelmap_t> tokenize_file(std::ifstream& infile, const insmap_t& insmap);
std::string ins2str(int pc, uint16_t iword);

bool optype_equal(const std::string& op, const std::string& type);

uint16_t num_parse(const std::string& instr);

uint16_t reg_parse(const std::string& reg_name, std::unordered_map<std::string, std::string> opfield_map, int pc);
uint16_t imm_parse(const std::string& imm_op, std::unordered_map<std::string, std::string> opfield_map, int pc);
uint16_t label_parse(const std::string& label, std::unordered_map<std::string, std::string> opfield_map, int pc);
uint16_t no_parse(const std::string& op, std::unordered_map<std::string, std::string> opfield_map, int pc);


insmap_t insmap_gen(const std::string& conf_file);
oplist_t opvec_gen(std::ifstream& cfile, int numops);
std::unordered_map<std::string, std::string> reg_opgen(std::ifstream& cfile);
std::unordered_map<std::string, std::string> imm_opgen(std::ifstream& cfile);
std::unordered_map<std::string, std::string> const_opgen(std::ifstream& cfile);
std::unordered_map<std::string, std::string> no_opgen(std::ifstream& cfile);


std::unordered_map<std::string, std::function<std::unordered_map<std::string, std::string>(std::ifstream&)>> opvec_gen_fns {
	{"reg", reg_opgen},
	{"imm", imm_opgen},
	{"label", no_opgen},
	{"pcx", const_opgen},
	{"flags", const_opgen},
};

std::unordered_map<std::string, std::function<uint16_t(const std::string&, std::unordered_map<std::string, std::string>, int)>> optype_fns {
	{"reg", reg_parse},
	{"imm", imm_parse},
	{"label", label_parse},
	{"pcx", no_parse},
	{"flags", no_parse},
};

std::unordered_map<std::string, int> label_map;

int main(int argc, char *argv[]) {

	std::string insfile = def_insfile;
	std::string outfile_name = def_outfile;
	std::string infile_name = "";
	// command line argument parsing
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'o') {
				if (i + 1 < argc) {
					outfile_name = argv[++i];
				} else {
					usage();
				}
			} else if (argv[i][1] == 'c') {
				if (i + 1 < argc) {
					insfile = argv[++i];
				} else {
					usage();
				}
			} else {
				std::cerr << "Unrecognized option " << argv[i] << std::endl;
				usage();
			}
		} else {
			infile_name = argv[i];
		}
	}

	if (infile_name == "") {
		usage();
	}

	insmap_t insmap = insmap_gen(insfile);

	std::ifstream infile {infile_name};
	if (!infile.is_open())
		error("can't open input file '" + infile_name + "'");

	auto [tok_vec, label_map] = tokenize_file(infile, insmap);
	infile.close();

	std::ofstream outfile {outfile_name};
	if (!outfile.is_open())
		error("can't open output file '" + outfile_name + "'");


	int pc = 0, iword;
	int line = 0;
	std::vector<oplist_t> ins_alts;
	for (const auto& tokens : tok_vec) {
		line++;
		if (tokens[0] == "org") {
			pc = num_parse(tokens[1]);
			continue;
		} else if (insmap.find(tokens[0]) == insmap.end()) {
			error("line " + std::to_string(line) + ": unknown instruction '" + tokens[0] + "'");
		}

		ins_alts = insmap[tokens[0]].first;
		iword = insmap[tokens[0]].second;
		if (ins_alts.size() != 0) {

			int alt_idx = 0;
			// skip to first instruction alternative with correct number
			// of operands
			while (ins_alts[alt_idx].size() != tokens.size() - 1) {
				alt_idx++;
				if (alt_idx >= ins_alts.size())
					error("line " + std::to_string(line) + ": no matching version of instruction '" + tokens[0] + "' found");
			}
			// select first alternative with matching number of operands
			if (ins_alts[alt_idx].size() == tokens.size() - 1) {

				// these indexes can differ if a 2 operand shorthand
				// of a 3 operand instruction is encountered:
				// index of operand in current alternative
				int alt_op = 0;
				// index of operand in current tokens vector
				int tok_op = 1; // starts at first operand in token vector
				// needed to skip already processed Ra if we go back
				// by one operand for Rc of 3 operand instruction
				// with 2 operand shorthand
				bool alt_op_skip = false;
				for (; tok_op < tokens.size(); tok_op++, alt_op++) {
					// go to next alternative if current operand does not match requirement
					while (!optype_equal(tokens[tok_op], ins_alts[alt_idx][alt_op]["type"])) {
						alt_idx++;
						if (alt_idx >= ins_alts.size()) {
							error("line " + std::to_string(line) + ": no matching version of instruction '" + tokens[0] + "' found");
						}
						if (ins_alts[alt_idx].size() > tokens.size() - 1) {
							// try 3 operand instruction by duplicating first operand
							if (ins_alts[alt_idx].size() == tokens.size()) {
								tok_op--;
								alt_op = tok_op - 1;
								alt_op_skip = true;
								if (tok_op == 0) {
									error("line " + std::to_string(line) + ": no matching version of instruction '" + tokens[0] + "' found");
								}
							} else {
								error("line " + std::to_string(line) + ": no matching version of instruction '" + tokens[0] + "' found");
							}
						}
					}


					iword += optype_fns[ins_alts[alt_idx][alt_op]["type"]](tokens[tok_op], ins_alts[alt_idx][alt_op], pc);


					if (alt_op_skip) {
						alt_op++;
						alt_op_skip = false;
					}
				}
			} else {
				error("line " + std::to_string(line) + ": no matching version of instruction '" + tokens[0] + "' found");
			}
		}

		outfile << ins2str(pc, iword) << "\n";
		pc++;
	}
 	outfile.close();
}

void usage() {
	std::cerr << "Usage: eepasm [-o outfile] [-c configfile] infile" << std::endl;
	std::exit(EXIT_FAILURE);
}

void error(const std::string& msg) {
	std::cerr << "Error: " << msg << std::endl;
	std::exit(EXIT_FAILURE);
}

void parsing_error(const std::string& msg, const std::string& insname) {
	std::cerr << "Parsing (instruction: " << insname << "): " << msg << std::endl;
	std::exit(EXIT_FAILURE);
}

void assem_error(const std::string& msg, const std::string& insname, int line) {
	std::cerr << "Assemble line " << line << " (instruction: " << insname << "): " << msg << std::endl;
	std::exit(EXIT_FAILURE);
}

// needs to have std::istream and not ifstream to work for string streams too
std::string get_low_str(std::istream& infile) {
	std::string out;
	infile >> out;
	std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
	return out;
}


insmap_t insmap_gen(const std::string& conf_file) {
	std::ifstream cfile {conf_file};
	if (!cfile.is_open())
		error("Can't open instruction list config file '" + conf_file + "'");

	insmap_t outmap;
	std::vector<oplist_t> alternatives_vec;
	std::string ins_name, instr;
	int numops;
	int line = 0;
	while ((ins_name = get_low_str(cfile)) != "") {
		// process instructions stored in instr
	
		instr = get_low_str(cfile);
		if (instr == "copy") {
			outmap[ins_name].first = alternatives_vec;
			instr = get_low_str(cfile);
		} else if (instr == "numops") {
			alternatives_vec.clear();
			while (instr == "numops") {
				cfile >> numops; // numops value
				if (numops > 3)
					parsing_error("can't have more than 3 operands", ins_name);
				alternatives_vec.push_back(opvec_gen(cfile, numops));
				instr = get_low_str(cfile);
			}
			outmap[ins_name].first = alternatives_vec;
		}

		// while already read string const_iword
		if (instr != "const_iword")
			parsing_error("missing const_iword field", ins_name);
		instr = get_low_str(cfile); // string of const_iword
		outmap[ins_name].second = num_parse(instr);
	}

	return outmap;
}

oplist_t opvec_gen(std::ifstream& cfile, int numops) {
	oplist_t outvec;
	std::unordered_map<std::string, std::string> opfield_map;
	std::string instr, type;


	for (int i = 0; i < numops; i++) {
		instr = get_low_str(cfile);
		if (instr != "op")
			error("parsing: missing op indicator");

		instr = get_low_str(cfile);
		if (instr != "type")
			error("parsing: type field missing");

		type = get_low_str(cfile);
		if (opvec_gen_fns.find(type) == opvec_gen_fns.end())
			error("parsing: invaid operand type");

		opfield_map = opvec_gen_fns[type](cfile);
		opfield_map["type"] = type;
	
		outvec.push_back(opfield_map);
	}
	return outvec;
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
		error("parsing: ins8 value must be 0 or 1");

	return opfield_map;
}

std::unordered_map<std::string, std::string> const_opgen(std::ifstream& cfile) {
	std::unordered_map<std::string, std::string>  opfield_map;

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
		error("parsing: '" + field_name + "' field missing");
	instr = get_low_str(cfile);
	return instr;
}

std::pair<tokvec_t, labelmap_t> tokenize_file(std::ifstream& infile, const insmap_t& insmap) {
	tokvec_t outvec;
	labelmap_t labelmap;

	std::string line, token;
	std::istringstream line_stream;
	std::vector<std::string> token_vec;
	int pc = 0;
	while (getline(infile, line)) {
		line_strip(line);
		if (line == "")
			continue;
		line_stream.clear(); // clear state flags
		line_stream.str(line);
		token_vec.clear();

		token = get_low_str(line_stream);
		if (insmap.find(token) == insmap.end() && token != "org") {
			label_map[token] = pc;
			std::string lower_stream_str = line_stream.str();
			transform(lower_stream_str.begin(), lower_stream_str.end(), lower_stream_str.begin(), [](char c) { return tolower(c); });
			if (lower_stream_str == token) { // if label on separate line
				continue; // need to skip incrementing pc
			}
		} else {
			token_vec.push_back(token);
		}
		if (token == "org") {
			token = get_low_str(line_stream);
			pc = num_parse(token);
			token_vec.push_back(token);
		} else {
			pc++;
				
			while ((token = get_low_str(line_stream)) != "") {
				while (token[0] == '#' || token [0] == '[')
					token = token.replace(0, 1, "");
				if (token[token.size()-1] == ',' || token[token.size()-1] == ']')
					token = token.replace(token.size()-1, token.size(), "");
				token_vec.push_back(token);
			}
		}
		outvec.push_back(token_vec);
	}
	return make_pair(outvec, labelmap);
}

bool optype_equal(const std::string& op, const std::string& type) {
	if (type == "reg") {
		return (op[0] == 'r');
	} else if (type == "imm") {
		// last case to also make it work for negative numbers
		return ((op[0] >= '0' && op[0] <= '9') || op[0] == '-');
	} else if (type == "label") {
		return true;
	} else if (type == "pcx") {
		return (op == "pcx");
	} else if (type == "flags") {
		return (op == "flags");
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

uint16_t reg_parse(const std::string& reg_name, std::unordered_map<std::string, std::string> opfield_map, int pc) {
	return (reg_name[1] - '0') << num_parse(opfield_map["lsb"]);
}

uint16_t imm_parse(const std::string& imm_op, std::unordered_map<std::string, std::string> opfield_map, int pc) {
	uint16_t bitmask = (1 << num_parse(opfield_map["size"])) - 1;
	uint16_t num = num_parse(imm_op);
	num = (num & bitmask) << num_parse(opfield_map["lsb"]);
	num += (num_parse(opfield_map["ins8"]) << 8);
	return num;
}

uint16_t label_parse(const std::string& label, std::unordered_map<std::string, std::string> opfield_map, int pc) {
	if (label_map.find(label) == label_map.end())
		error("assembly: label not found in program");
	return (label_map[label] - static_cast<uint16_t>(pc)) & 0xff;
}

uint16_t no_parse(const std::string& instr, std::unordered_map<std::string, std::string> opfield_map, int pc) {
	return 0;
}
