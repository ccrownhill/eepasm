#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdint>
#include <unordered_map>
#include <functional> // for function objects
#include <vector>

void error(const std::string& msg);
void line_strip(std::string& line);
std::vector<std::vector<std::string>> tokenize_file(std::ifstream& infile);
std::string ins2str(int pc, uint16_t iword);
uint16_t reg_parse(const std::string& reg_name);
uint16_t imm_parse(const std::string& imm_op, uint16_t bitmask);
uint16_t mov_parse(const std::vector<std::string>& tokens);
uint16_t no_parse(const std::vector<std::string>& tokens, uint16_t const_iword, int pc);
uint16_t alu_parse(const std::vector<std::string>& tokens, uint16_t const_iword, int pc);
uint16_t shift_parse(const std::vector<std::string>& tokens, uint16_t const_iword, int pc);
uint16_t mem_parse(const std::vector<std::string>& tokens, uint16_t const_iword, int pc);
uint16_t jmp_parse(const std::vector<std::string>& tokens, uint16_t const_iword, int pc);
std::unordered_map<std::string, uint16_t> get_label_map(const std::ifstream& infile);


std::unordered_map<std::string, std::function<uint16_t(const std::vector<std::string>&, uint16_t, int)>> opmap {
	{"MOV", alu_parse},
	{"ADD", alu_parse},
	{"SUB", alu_parse},
	{"ADC", alu_parse},
	{"SBC", alu_parse},
	{"AND", alu_parse},
	{"CMP", alu_parse},
	{"LSL", shift_parse},
	{"LSR", shift_parse},
	{"ASR", shift_parse},
	{"XSR", shift_parse},
	{"LDR", mem_parse},
	{"STR", mem_parse},
	{"JMP", jmp_parse},
	{"NOOP", jmp_parse},
	{"JEQ", jmp_parse},
	{"JNE", jmp_parse},
	{"JCS", jmp_parse},
	{"JCC", jmp_parse},
	{"JMI", jmp_parse},
	{"JPL", jmp_parse},
	{"JGE", jmp_parse},
	{"JLT", jmp_parse},
	{"JGT", jmp_parse},
	{"JLE", jmp_parse},
	{"JHI", jmp_parse},
	{"JLS", jmp_parse},
	{"JSR", jmp_parse},
	{"RET", no_parse},
};

std::unordered_map<std::string, int> label_map;

int main(int argc, char *argv[]) {
	if (argc != 3)
		error("Usage: eepasm infile outfile");

	std::ifstream infile {argv[1]};
	if (!infile)
		error("can't open input file");

	std::vector<std::vector<std::string>> tok_vec = tokenize_file(infile);
	infile.close();

	std::ofstream outfile {argv[2]};
	if (!outfile)
		error("can't open output file");


	int pc = 0, iword;
	for (const auto& tokens : tok_vec) {
		if (tokens[0] == "MOV" || tokens[0] == "mov") {
			iword = 0;
		} else if (tokens[0] == "ADD" || tokens[0] == "add") {
			iword = 0b0001000000000000;
		} else if (tokens[0] == "SUB" || tokens[0] == "sub") {
			iword = 0b0010000000000000;
		} else if (tokens[0] == "ADC") {
			iword = 0b0011000000000000;
		} else if (tokens[0] == "SDC") {
			iword = 0b0100000000000000;
		} else if (tokens[0] == "AND") {
			iword = 0b0101000000000000;
		} else if (tokens[0] == "CMP") {
			iword = 0b0110000000000000;
		} else if (tokens[0] == "LSL") {
			iword = 0b0111000000000000;
		} else if (tokens[0] == "LSR") {
			iword = 0b0111000000010000;
		} else if (tokens[0] == "ASR") {
			iword = 0b0111000100000000;
		} else if (tokens[0] == "XSR") {
			iword = 0b0111000100010000;
		} else if (tokens[0] == "LDR") {
			iword = 0b1000000000000000;
		} else if (tokens[0] == "STR") {
			iword = 0b1010000000000000;
		} else if (tokens[0] == "JMP") {
			iword = 0b1100000000000000;
		} else if (tokens[0] == "NOOP") {
			iword = 0b1100000100000000;
		} else if (tokens[0] == "JEQ") {
			iword = 0b1100001000000000;
		} else if (tokens[0] == "JNE") {
			iword = 0b1100001100000000;
		} else if (tokens[0] == "JCS") {
			iword = 0b1100010000000000;
		} else if (tokens[0] == "JCC") {
			iword = 0b1100010100000000;
		} else if (tokens[0] == "JMI") {
			iword = 0b1100011000000000;
		} else if (tokens[0] == "JPL") {
			iword = 0b1100011100000000;
		} else if (tokens[0] == "JGE") {
			iword = 0b1100100000000000;
		} else if (tokens[0] == "JLT") {
			iword = 0b1100100100000000;
		} else if (tokens[0] == "JGT") {
			iword = 0b1100101000000000;
		} else if (tokens[0] == "JLE") {
			iword = 0b1100101100000000;
		} else if (tokens[0] == "JHI") {
			iword = 0b1100110000000000;
		} else if (tokens[0] == "JLS") {
			iword = 0b1100110100000000;
		} else if (tokens[0] == "JSR") {
			iword = 0b1100111000000000;
		} else if (tokens[0] == "RET") {
			iword = 0b1100111100000000;
		} else if (tokens[0] == "EXT") {
			iword = 0b1101000000000000;
		} else {
			error("Unknown instruction");
		}
		iword = opmap[tokens[0]](tokens, iword, pc);
		outfile << ins2str(pc, iword) << "\n";
		pc++;
	}
	outfile.close();
}

void error(const std::string& msg) {
	std::cerr << "Error: " << msg << std::endl;
	std::exit(EXIT_FAILURE);
}


std::vector<std::vector<std::string>> tokenize_file(std::ifstream& infile) {
	std::string line, token;
	std::istringstream line_stream;
	std::vector<std::vector<std::string>> outvec;
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
		if (opmap.find(token) == opmap.end()) {
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
	return outvec;
}


std::unordered_map<std::string, uint16_t> get_label_map(const std::ifstream& infile) {
	return {};
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

uint16_t reg_parse(const std::string& reg_name) {
	return reg_name[1] - '0';
}

// parses the given immediate operand with the leading #
// returns the number after the # anded with bitmask
uint16_t imm_parse(const std::string& imm_op, uint16_t bitmask) {
	uint16_t num;
	if (imm_op.substr(0,2) == "0x")
		num = std::stoi(imm_op.substr(2), nullptr, 16); // base 16
	else if (imm_op.substr(0,2) == "0b")
		num = std::stoi(imm_op.substr(2), nullptr, 2); // base 2
	else
		num = std::stoi(imm_op); // base 10
	return num & bitmask;
}

uint16_t no_parse(const std::vector<std::string>& tokens, uint16_t const_iword, int pc) {
	return const_iword;
}

uint16_t alu_parse(const std::vector<std::string>& tokens, uint16_t const_iword, int pc) {
	uint16_t iword = const_iword;
	
	// first operand (has to be register)
	if (tokens.size() == 4) // Rc first operand
		iword += reg_parse(tokens[1]) << 2;
	else	// Ra first operand
		iword += reg_parse(tokens[1]) << 9;

	// second operand (can be register or immediate value)
	if (tokens.size() == 3) {
		if (tokens[2][0] == 'R') {
		    iword += reg_parse(tokens[2]) << 5; // put in Rb position
		} else {
		    iword += 1 << 8; // set iword[8] bit
		    iword += imm_parse(tokens[2], 0xff); // mask for 8 bit immediate
		}
	} else {
		iword += reg_parse(tokens[2]) << 9; // Ra
		
		iword += reg_parse(tokens[3]) << 5; // Rb
	}
	return iword;
}

uint16_t shift_parse(const std::vector<std::string>& tokens, uint16_t const_iword, int pc) {
	uint16_t iword = const_iword;
	
	// first operand: Ra
	iword += reg_parse(tokens[1]) << 9;

	// second operand: Rb
	iword += reg_parse(tokens[2]) << 5;

	// third operand: 4 bit immediate
	iword += imm_parse(tokens[3], 0xf); // mask for 4 bit immediate
	return iword;
}

uint16_t mem_parse(const std::vector<std::string>& tokens, uint16_t const_iword, int pc) {
	uint16_t iword = const_iword;

	// first operand: Ra
	iword += reg_parse(tokens[1]) << 9;

	if (tokens[2][0] == 'R' ) {
		iword += reg_parse(tokens[2]) << 5;
		if (tokens.size() == 4) { // check for Imms5 offset
			iword += imm_parse(tokens[3], 0b11111); // 5 bit bitmask
		}

	} else {
		iword += 1 << 8; // set iword[8] bit
		iword += imm_parse(tokens[2], 0xff); // 8 bit bitmask
	}
	return iword;
}

uint16_t jmp_parse(const std::vector<std::string>& tokens, uint16_t const_iword, int pc) {
	uint16_t iword = const_iword;
	if (tokens[1][0] >= '0' && tokens[1][0] <= '9') // check if there is a numeric value
		iword += imm_parse(tokens[1], 0xff); // 8 bit mask
	else // means that there is a label
		iword += (label_map[tokens[1]] - static_cast<uint16_t>(pc)) & 0xff;
	return iword;
}
