#include <iostream>
#include <fstream>
#include <unordered_map>
#include <functional> // for function objects
#include <vector>
#include <utility> // for pair
#include <algorithm> // for transform
#include <stdexcept>
#include <cstdint>
#include <string>
#include <sstream>

#include "eepasm.h"

std::unordered_map<std::string, std::function<bool(const std::string&, std::unordered_map<std::string, std::string>&)>> optype_check_fns {
	{"reg", reg_check},
	{"imm", imm_check},
	{"label", label_check},
	{"lit", lit_check},
};


std::unordered_map<std::string, std::function<std::unordered_map<std::string, std::string>(std::ifstream&)>> opvec_gen_fns {
	{"reg", reg_opgen},
	{"imm", imm_opgen},
	{"label", no_opgen},
	{"lit", lit_opgen},
};

std::unordered_map<std::string, std::function<uint16_t(const std::string&, std::unordered_map<std::string, std::string>&, int)>> optype_fns {
	{"reg", reg_parse},
	{"imm", imm_parse},
	{"label", label_parse},
	{"lit", lit_parse},
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
		try {
			if (tokens[0] == "org") {
				pc = num_parse(tokens[1]);
				continue;
			} else if (insmap.find(tokens[0]) == insmap.end()) {
				throw assem_error {"unknown instruction"};
			}

			ins_alts = insmap[tokens[0]].first;
			iword = insmap[tokens[0]].second;
			if (ins_alts.size() != 0) {

				int alt_idx = 0;
				// skip to first instruction alternative with correct number
				// of operands
				while (ins_alts[alt_idx].size() != tokens.size() - 1) {
					alt_idx++;
					if (alt_idx >= ins_alts.size()) {
						throw assem_error {"no matching version of instruction found"};
					}
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
						while (!optype_check_fns[ins_alts[alt_idx][alt_op]["type"]](tokens[tok_op], ins_alts[alt_idx][alt_op])) {
							alt_idx++;
							if (alt_idx >= ins_alts.size()) {
								throw assem_error {"no matching version of instruction found"};
							}
							if (ins_alts[alt_idx].size() > tokens.size() - 1) {
								// try 3 operand instruction by duplicating first operand
								if (ins_alts[alt_idx].size() == tokens.size()) {
									tok_op--;
									alt_op = tok_op - 1;
									alt_op_skip = true;
									if (tok_op == 0) {
										throw assem_error {"no matching version of instruction found"};
									}
								} else {
									throw assem_error {"no matching version of instruction found"};
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
					throw assem_error {"no matching version of instruction found"};
				}
			}

			outfile << ins2str(pc, iword) << "\n";
			pc++;
		} catch (const assem_error& err) {
			error("assembly instruction " + std::to_string(line) + " (" + tokens[0] + "): " + err.what());
		}
	}
 	outfile.close();
}

void usage() {
	error("Usage: eepasm [-o outfile] [-c configfile] infile");
}

void error(const std::string& msg) {
	std::cerr << "Error: " << msg << std::endl;
	std::exit(EXIT_FAILURE);
}



insmap_t insmap_gen(const std::string& conf_file) {
	std::ifstream cfile {conf_file};
	if (!cfile.is_open())
		error("Can't open instruction list config file '" + conf_file + "'");

	insmap_t outmap;
	std::vector<oplist_t> alternatives_vec;
	std::string ins_name, instr;
	int numops;

	while ((ins_name = get_low_str(cfile)) != "") {
		// process instructions stored in instr
	
		try {
			instr = get_low_str(cfile);
			if (instr == "copy") {
				outmap[ins_name].first = alternatives_vec;
				instr = get_low_str(cfile);
			} else if (instr == "numops") {
				alternatives_vec.clear();
				while (instr == "numops") {
					cfile >> numops; // numops value
					if (numops > 3)
						throw parsing_error {"can't have more than 3 operands"};
					alternatives_vec.push_back(opvec_gen(cfile, numops));
					instr = get_low_str(cfile);
				}
				outmap[ins_name].first = alternatives_vec;
			}

			// while already read string const_iword
			if (instr != "const_iword")
				throw parsing_error {"missing const_iword field"};
			instr = get_low_str(cfile); // string of const_iword
			outmap[ins_name].second = num_parse(instr);
		} catch (const parsing_error& err) {
			error("parsing (" + ins_name + "): " + err.what());
		}
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
			throw parsing_error {"missing op indicator"};

		instr = get_low_str(cfile);
		if (instr != "type")
			throw parsing_error {"type field missing"};

		type = get_low_str(cfile);
		if (opvec_gen_fns.find(type) == opvec_gen_fns.end())
			throw parsing_error {"invaid operand type"};

		opfield_map = opvec_gen_fns[type](cfile);
		opfield_map["type"] = type;
	
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
