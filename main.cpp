#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdint>

void error(const std::string& msg);
void comment_strip(std::string& line);
std::string ins2str(uint16_t pc, uint16_t iword);
uint16_t reg_parse(const std::string& reg_name);
uint16_t imm_parse(const std::string& imm_op, uint16_t bitmask);
uint16_t mov_parse(const std::string& operands);
uint16_t alu_parse(const std::string& operands, uint16_t const_iword);
uint16_t shift_parse(const std::string& operands, uint16_t const_iword);
uint16_t mem_parse(const std::string& operands, uint16_t const_iword);
uint16_t jmp_parse(const std::string& operands, uint16_t const_iword);

int main(int argc, char *argv[]) {
	if (argc != 3)
		error("Usage: eepasm infile outfile");

	std::ifstream infile {argv[1]};
	if (!infile)
		error("can't open input file");
	std::ofstream outfile {argv[2]};
	if (!outfile)
		error("can't open output file");

	std::string opcode;
	std::string operands;
	uint16_t pc = 0, iword;
	while (infile >> opcode && getline(infile, operands)) {
		operands = operands.substr(1); // strip leading space
		comment_strip(operands);
		if (opcode == "MOV" || opcode == "mov") {
			iword = 0;
			iword = alu_parse(operands, iword);
		} else if (opcode == "ADD" || opcode == "add") {
			iword = 0b0001000000000000;
			iword = alu_parse(operands, iword);
		} else if (opcode == "SUB" || opcode == "sub") {
			iword = 0b0010000000000000;
			iword = alu_parse(operands, iword);
		} else if (opcode == "ADC") {
			iword = 0b0011000000000000;
			iword = alu_parse(operands, iword);
		} else if (opcode == "SDC") {
			iword = 0b0100000000000000;
			iword = alu_parse(operands, iword);
		} else if (opcode == "AND") {
			iword = 0b0101000000000000;
			iword = alu_parse(operands, iword);
		} else if (opcode == "CMP") {
			iword = 0b0110000000000000;
			iword = alu_parse(operands, iword);
		} else if (opcode == "LSL") {
			iword = 0b0111000000000000;
			iword = shift_parse(operands, iword);
		} else if (opcode == "LSR") {
			iword = 0b0111000000010000;
			iword = shift_parse(operands, iword);
		} else if (opcode == "ASR") {
			iword = 0b0111000100000000;
			iword = shift_parse(operands, iword);
		} else if (opcode == "XSR") {
			iword = 0b0111000100010000;
			iword = shift_parse(operands, iword);
		} else if (opcode == "LDR") {
			iword = 0b1000000000000000;
			iword = mem_parse(operands, iword);
		} else if (opcode == "STR") {
			iword = 0b1010000000000000;
			iword = mem_parse(operands, iword);
		} else if (opcode == "JMP") {
			iword = 0b1100000000000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "NOOP") {
			iword = 0b1100000100000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "JEQ") {
			iword = 0b1100001000000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "JNE") {
			iword = 0b1100001100000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "JCS") {
			iword = 0b1100010000000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "JCC") {
			iword = 0b1100010100000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "JMI") {
			iword = 0b1100011000000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "JPL") {
			iword = 0b1100011100000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "JGE") {
			iword = 0b1100100000000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "JLT") {
			iword = 0b1100100100000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "JGT") {
			iword = 0b1100101000000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "JLE") {
			iword = 0b1100101100000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "JHI") {
			iword = 0b1100110000000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "JLS") {
			iword = 0b1100110100000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "JSR") {
			iword = 0b1100111000000000;
			iword = jmp_parse(operands, iword);
		} else if (opcode == "RET") {
			iword = 0b1100111100000000;
		} else if (opcode == "EXT") {
			iword = 0b1101000000000000;
			iword += imm_parse(operands, 0xff); // 8 bit mask
		} else {
			error("Unknown instruction");
		}
		outfile << ins2str(pc++, iword) << "\n";
	}
}

void error(const std::string& msg) {
	std::cerr << "Error: " << msg << std::endl;
	std::exit(EXIT_FAILURE);
}

void comment_strip(std::string& line) {
	int comment_start = line.find("//");
	if (comment_start != -1)
		line = line.replace(comment_start, line.size()-1, "");
}

std::string ins2str(uint16_t pc, uint16_t iword) {
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

// counts whether instruction has 2 or 3 operands
// does this by finding last occurrence of ","
// if it is beyond index 2 (last occurrence of "," in 2 operand instruction)
// it must be a 3 operand instruction
uint16_t count_operands(const std::string& operands) {
	return (operands.rfind(",") > 2) ? 3 : 2;
}

uint16_t reg_parse(const std::string& reg_name) {
	return reg_name[1] - '0';
}

// parses the given immediate operand with the leading #
// returns the number after the # anded with bitmask
uint16_t imm_parse(const std::string& imm_op, uint16_t bitmask) {
	uint16_t num;
	if (imm_op.substr(0,3) == "#0x")
		num = std::stoi(imm_op.substr(3), nullptr, 16); // base 16
	else if (imm_op.substr(0,3) == "#0b")
		num = std::stoi(imm_op.substr(3), nullptr, 2); // base 2
	else
		num = std::stoi(imm_op.substr(1)); // base 10
	return num & bitmask;
}

uint16_t mov_parse(const std::string& operands) {
	std::istringstream op_stream {operands};
	std::string operand;
	uint16_t iword = 0x0;
	
	// first operand (has to be register)
	op_stream >> operand;
	operand.replace(2, 3, ""); // remove trailing comma
	iword += reg_parse(operand) << 9;

	// second operand (can be register or immediate value)
	op_stream >> operand;
	if (operand[0] == '#') {
	    iword += 1 << 8; // set iword[8] bit
	    iword += imm_parse(operand, 0xff); // mask for 8 bit immediate
	} else {
	    iword += reg_parse(operand) << 5;
	}
	return iword;
}


uint16_t alu_parse(const std::string& operands, uint16_t const_iword) {
	std::istringstream op_stream {operands};
	std::string operand;
	uint16_t iword = const_iword;
	uint16_t op_count = count_operands(operands);
	
	// first operand (has to be register)
	op_stream >> operand;
	operand.replace(2, 3, ""); // remove trailing comma
	if (op_count == 3) // Rc first operand
		iword += reg_parse(operand) << 2;
	else	// Ra first operand
		iword += reg_parse(operand) << 9;

	// second operand (can be register or immediate value)
	if (op_count == 2) {
		op_stream >> operand;
		if (operand[0] == '#') {
		    iword += 1 << 8; // set iword[8] bit
		    iword += imm_parse(operand, 0xff); // mask for 8 bit immediate
		} else {
		    iword += reg_parse(operand) << 5; // put in Rb position
		}
	} else {
		op_stream >> operand;
		operand.replace(2, 3, "");
		iword += reg_parse(operand) << 9; // Ra
		
		op_stream >> operand;
		iword += reg_parse(operand) << 5; // Rb
	}
	return iword;
}

uint16_t shift_parse(const std::string& operands, uint16_t const_iword) {
	std::istringstream op_stream {operands};
	std::string operand;
	uint16_t iword = const_iword;
	
	// first operand: Ra
	op_stream >> operand;
	operand.replace(2, 3, ""); // remove trailing comma
	iword += reg_parse(operand) << 9;

	// second operand: Rb
	op_stream >> operand;
	operand.replace(2, 3, ""); // remove trailing comma
	iword += reg_parse(operand) << 5;

	// third operand: 4 bit immediate
	op_stream >> operand;
	iword += imm_parse(operand, 0xf); // mask for 4 bit immediate

	return iword;
}

uint16_t mem_parse(const std::string& operands, uint16_t const_iword) {
	std::istringstream op_stream {operands};
	std::string operand;
	uint16_t iword = const_iword;

	// first operand: Ra
	op_stream >> operand;
	operand.replace(2, 3, ""); // remove trailing comma
	iword += reg_parse(operand) << 9;

	op_stream >> operand;
	operand.replace(0, 1, ""); // remove [
	if (operand[0] == '#' ) { // single Imms8 address
		operand.replace(operand.size()-1, operand.size(), ""); // remove ]
		iword += 1 << 8; // set iword[8] bit
		iword += imm_parse(operand, 0xff); // 8 bit bitmask
	} else {
		operand.replace(3, 4, ""); // remove trailing ] or ,
		iword += reg_parse(operand) << 5;
		if (op_stream >> operand) { // check for Imms5 offset
			operand.replace(operand.size()-1, operand.size(), ""); // remove ]
			iword += imm_parse(operand, 0b11111); // 5 bit bitmask
		}
	}
	return iword;
}

uint16_t jmp_parse(const std::string& operands, uint16_t const_iword) {
	uint16_t iword = const_iword;

	if (operands[0] == '#')
		iword += imm_parse(operands, 0xff); // 8 bit mask
	else
		iword += imm_parse("#" + operands, 0xff);
	return iword;
}
