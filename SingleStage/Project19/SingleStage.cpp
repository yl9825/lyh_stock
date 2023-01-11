#include<iostream>
#include<string>
#include<vector>
#include<bitset>
#include<fstream>
#include<istream>

using namespace std;

#define MemSize 1000 // memory size, in reality, the memory size should be 2^32, but for this lab, for the space resaon, we keep it as this large number, but the memory is still 32-bit addressable.

bitset<32> add(bitset<32> A, bitset<32> B)
{
	int inc = 0;
	bitset<32> ans;
	for (int i = 0; i < 32; i++)
	{
		if (A[i] + B[i] + inc == 3)
		{
			ans[i] = 1;
			inc = 1;
		}
		else if (A[i] + B[i] + inc == 2)
		{
			ans[i] = 0;
			inc = 1;
		}
		else if (A[i] + B[i] + inc == 1)
		{
			ans[i] = 1;
			inc = 0;
		}
		else if (A[i] + B[i] + inc == 0)
		{
			ans[i] = 0;
			inc = 0;
		}
	}
	return ans;
}

bitset<32> sub(bitset<32> A, bitset<32> B)
{
	B.flip();
	B = add(B, bitset<32>(1));
	return add(A, B);
}

struct IFStruct {
	bitset<32>  PC;
	bool        jump;
	bitset<32>  jump_value;
	bool        nop;
};

struct IDStruct {
	bitset<32>  Instr;
	bool        nop;
	//bool        jump;//new
};

struct EXStruct {
	bitset<32>  Read_data1;
	bitset<32>  Read_data2;
	bitset<32>  Imm;
	bitset<5>   Rs;//r1
	bitset<5>   Rt;//r2
	bitset<5>   Wrt_reg_addr;//write register
	bitset<3>   ALU_control;//new: 0 add   1 sub   2 xor   3 or   4 and
	bool        is_I_type;//ALUsrc
	//bool        rd_mem;
	//bool        wrt_mem;
	//bool        alu_op;     //1 for addu, lw, sw, 0 for subu 
	//bool        wrt_enable;
	bool        nop;
};

struct MEMStruct {
	bitset<32>  ALUresult;
	bitset<32>  Store_data;
	//bitset<5>   Rs;
	//bitset<5>   Rt;
	//bitset<5>   Wrt_reg_addr;
	bool        rd_mem;
	bool        wrt_mem;
	//bool        wrt_enable;//writeback
	bool        nop;
};

struct WBStruct {
	bitset<32>  Wrt_data;
	//bitset<5>   Rs;
	//bitset<5>   Rt;
	bitset<5>   Wrt_reg_addr;
	//bool        wrt_enable;
	//bool        Mem_to_Reg;//1 from mem, 0 from ALU
	bool        nop;
};

struct stateStruct {
	IFStruct    IF;
	IDStruct    ID;
	EXStruct    EX;
	MEMStruct   MEM;
	WBStruct    WB;
};

class InsMem
{
public:
	string id, ioDir;
	int i = 0;
	InsMem(string name, string ioDir) {
		id = name;
		IMem.resize(MemSize);
		ifstream imem;
		string line;
		//int i = 0;
		imem.open(ioDir + "\\imem.txt");
		if (imem.is_open())
		{
			while (getline(imem, line))
			{
				IMem[i] = bitset<8>(line);
				i++;
			}
		}
		else cout << "Unable to open IMEM input file.";
		imem.close();
	}

	bitset<32> readInstr(bitset<32> ReadAddress) {
		// read instruction memory
		string temp = IMem[ReadAddress.to_ulong()].to_string() + IMem[ReadAddress.to_ulong() + 1].to_string() +
			IMem[ReadAddress.to_ulong() + 2].to_string() + IMem[ReadAddress.to_ulong() + 3].to_string();
		return bitset<32>(temp);
	}

private:
	vector<bitset<8> > IMem;
};

class DataMem
{
public:
	string id, opFilePath, ioDir;
	DataMem(string name, string ioDir) : id{ name }, ioDir{ ioDir } {
		DMem.resize(MemSize);
		opFilePath = ioDir + "\\" + name + "_DMEMResult.txt";
		ifstream dmem;
		string line;
		int i = 0;
		dmem.open(ioDir + "\\dmem.txt");
		if (dmem.is_open())
		{
			while (getline(dmem, line))
			{
				DMem[i] = bitset<8>(line);
				i++;
			}
		}
		else cout << "Unable to open DMEM input file.";
		dmem.close();
	}

	bitset<32> readDataMem(bitset<32> Address) {
		// read data memory
		// return bitset<32> val
		string temp = DMem[Address.to_ulong()].to_string() + DMem[Address.to_ulong() + 1].to_string() +
			DMem[Address.to_ulong() + 2].to_string() + DMem[Address.to_ulong() + 3].to_string();
		return bitset<32>(temp);
	}

	void writeDataMem(bitset<32> Address, bitset<32> WriteData) {
		// write into memory

		bitset<8> data0(WriteData.to_string().substr(0, 8));
		bitset<8> data1(WriteData.to_string().substr(8, 8));
		bitset<8> data2(WriteData.to_string().substr(16, 8));
		bitset<8> data3(WriteData.to_string().substr(24, 8));

		DMem[Address.to_ulong()] = data0;
		DMem[Address.to_ulong() + 1] = data1;
		DMem[Address.to_ulong() + 2] = data2;
		DMem[Address.to_ulong() + 3] = data3;
	}

	void outputDataMem() {
		ofstream dmemout;
		dmemout.open(opFilePath, std::ios_base::trunc);
		if (dmemout.is_open()) {
			for (int j = 0; j < 1000; j++)
			{
				dmemout << DMem[j] << endl;
			}

		}
		else cout << "Unable to open " << id << " DMEM result file." << endl;
		dmemout.close();
	}

private:
	vector<bitset<8> > DMem;
};

class RegisterFile
{
public:
	string outputFile;
	RegisterFile(string ioDir) : outputFile{ ioDir + "RFResult.txt" } {
		Registers.resize(32);
		Registers[0] = bitset<32>(0);
	}

	bitset<32> readRF(bitset<5> Reg_addr) {
		// Fill in
		return Registers[Reg_addr.to_ulong()];
	}

	void writeRF(bitset<5> Reg_addr, bitset<32> Wrt_reg_data) {
		// Fill in
		if (Reg_addr.to_ulong() == 0) return;
		Registers[Reg_addr.to_ulong()] = Wrt_reg_data;
	}

	void outputRF(int cycle) {
		ofstream rfout;
		if (cycle == 0)
			rfout.open(outputFile, std::ios_base::trunc);
		else
			rfout.open(outputFile, std::ios_base::app);
		if (rfout.is_open())
		{
			rfout << "State of RF after executing cycle:\t" << cycle << endl;
			for (int j = 0; j < 32; j++)
			{
				rfout << Registers[j] << endl;
			}
		}
		else cout << "Unable to open RF output file." << endl;
		rfout.close();
	}

private:
	vector<bitset<32> >Registers;
};

class Core {
public:
	RegisterFile myRF;
	uint32_t cycle = 0;
	bool halted = false;
	string ioDir;
	struct stateStruct state, nextState;
	InsMem ext_imem;
	DataMem ext_dmem;

	Core(string ioDir, InsMem& imem, DataMem& dmem) : myRF(ioDir), ioDir{ ioDir }, ext_imem{ imem }, ext_dmem{ dmem } {}

	virtual void step() {}

	virtual void printState() {}
};

class SingleStageCore : public Core {
public:
	SingleStageCore(string ioDir, InsMem& imem, DataMem& dmem) : Core(ioDir + "\\SS_", imem, dmem), opFilePath(ioDir + "\\StateResult_SS.txt") {}

	void step() {
		/* Your implementation*/
		//start

		if (cycle >= 0)
		{
			/* IF */
			if (state.IF.nop == true) halted = true;
			else
			{
				state.ID.Instr = ext_imem.readInstr(state.IF.PC);
				state.IF.jump = false;
			}
			/* ID */
			if (state.ID.Instr.to_string().substr(25, 7) == "1111111")//halt
			{
				state.IF.nop = true;
				state.EX.nop = true;
				state.MEM.nop = true;
				state.WB.nop = true;
				//halted = true;
				//return;				
			}
			else
			{
				if (state.ID.Instr.to_string().substr(25, 7) == "0110011")//add,sub,xor,and,or
				{
					state.EX.nop = false;
					state.EX.is_I_type = false;

					state.MEM.nop = true;
					//state.MEM.rd_mem = false;
					//state.MEM.wrt_mem = false;

					state.WB.nop = false;
					//state.WB.wrt_enable = true;
					//state.WB.Mem_to_Reg = false;

					state.EX.Rs = stoi(state.ID.Instr.to_string().substr(12, 5), 0, 2);
					state.EX.Rt = stoi(state.ID.Instr.to_string().substr(7, 5), 0, 2);
					state.EX.Read_data1 = myRF.readRF(state.EX.Rs); //readdata1= rs value
					state.EX.Read_data2 = myRF.readRF(state.EX.Rt); //readdata2= rt value

					state.EX.Wrt_reg_addr = stoi(state.ID.Instr.to_string().substr(20, 5), 0, 2);
					state.WB.Wrt_reg_addr = state.EX.Wrt_reg_addr;
					if (state.ID.Instr.to_string().substr(0, 7) == "0000000")//add,xor,and,or
					{
						if (state.ID.Instr.to_string().substr(17, 3) == "000")//add
						{
							state.EX.ALU_control = 0;
						}
						else if (state.ID.Instr.to_string().substr(17, 3) == "100")//xor
						{
							state.EX.ALU_control = 2;
						}
						else if (state.ID.Instr.to_string().substr(17, 3) == "110")//or
						{
							state.EX.ALU_control = 3;
						}
						else if (state.ID.Instr.to_string().substr(17, 3) == "111")//and
						{
							state.EX.ALU_control = 4;
						}
					}
					else if (state.ID.Instr.to_string().substr(0, 7) == "0100000"&& state.ID.Instr.to_string().substr(17, 3) == "000")//sub
					{
						state.EX.ALU_control = 1;
					}
				}
				else if (state.ID.Instr.to_string().substr(25, 7) == "0010011")//addi,xori,ori,andi
				{
					state.EX.nop = false;
					state.EX.is_I_type = true;

					state.MEM.nop = true;
					//state.MEM.rd_mem = false;
					//state.MEM.wrt_mem = false;

					state.WB.nop = false;
					//state.WB.wrt_enable = true;
					//state.WB.Mem_to_Reg = false;

					state.EX.Rs = stoi(state.ID.Instr.to_string().substr(12, 5), 0, 2);
					state.EX.Read_data1 = myRF.readRF(state.EX.Rs); //readdata1= rs value
					state.EX.Wrt_reg_addr = stoi(state.ID.Instr.to_string().substr(20, 5), 0, 2);
					state.WB.Wrt_reg_addr = state.EX.Wrt_reg_addr;
					string temp = state.ID.Instr.to_string().substr(0, 12);
					if (temp[0] == '0') state.EX.Imm = stoi(temp, 0, 2);
					else if (temp[0] == '1')
					{
						state.EX.Imm = stoi(temp, 0, 2);
						state.EX.Imm |= bitset<32>("11111111111111111111000000000000");
					}
					//state.EX.Imm = stoi(state.IF.PC.to_string().substr(0, 12), 0, 2);

					if (state.ID.Instr.to_string().substr(17, 3) == "000")//addi
					{
						state.EX.ALU_control = 0;
					}
					else if (state.ID.Instr.to_string().substr(17, 3) == "100")//xori
					{
						state.EX.ALU_control = 2;
					}
					else if (state.ID.Instr.to_string().substr(17, 3) == "110")//ori
					{
						state.EX.ALU_control = 3;
					}
					else if (state.ID.Instr.to_string().substr(17, 3) == "111")//andi
					{
						state.EX.ALU_control = 4;
					}
				}
				else if (state.ID.Instr.to_string().substr(25, 7) == "1101111")//jal
				{
					state.IF.jump = true;
					state.EX.nop = true;
					//state.EX.is_I_type = true;

					state.MEM.nop = true;
					//state.MEM.rd_mem = false;
					//state.MEM.wrt_mem = false;

					state.WB.nop = true;
					//state.WB.wrt_enable = false;

					state.EX.ALU_control = 0;
					myRF.writeRF(bitset<5>(state.ID.Instr.to_string().substr(20, 5)), bitset<32>(state.IF.PC.to_ulong() + 4));//rd = PC +4
					string imm = state.ID.Instr.to_string().substr(0, 20);
					imm = imm.substr(0, 1) + imm.substr(12, 8) + imm.substr(11, 1) + imm.substr(1, 10) + "0";
					if (imm[0] == '0')
					{
						state.EX.Imm = stoi(imm, 0, 2);
					}
					else if (imm[0] == '1')
					{
						state.EX.Imm = stoi(imm, 0, 2);
						state.EX.Imm |= bitset<32>("11111111111000000000000000000000");
					}
					state.IF.jump_value = state.EX.Imm;
					string ans;

				}
				else if (state.ID.Instr.to_string().substr(25, 7) == "1100011")//beq,bne
				{
					state.EX.Rs = stoi(state.ID.Instr.to_string().substr(12, 5), 0, 2);
					state.EX.Rt = stoi(state.ID.Instr.to_string().substr(7, 5), 0, 2);
					state.EX.Read_data1 = myRF.readRF(state.EX.Rs); //readdata1= rs value
					state.EX.Read_data2 = myRF.readRF(state.EX.Rt); //readdata2= rt value
					bool identical = (myRF.readRF(state.EX.Rs) == myRF.readRF(state.EX.Rt));

					if (state.ID.Instr.to_string().substr(17, 3) == "000")//beq
					{
						if (identical)
						{
							state.IF.jump = true;
							state.EX.nop = true;
							//state.EX.is_I_type = true;

							state.MEM.nop = true;
							//state.MEM.rd_mem = false;
							//state.MEM.wrt_mem = false;

							state.WB.nop = true;
							//state.WB.wrt_enable = false;

							//state.EX.ALU_control = 0;

							string imm = state.ID.Instr.to_string().substr(0, 1) + state.ID.Instr.to_string().substr(24, 1) +
								state.ID.Instr.to_string().substr(1, 6) + state.ID.Instr.to_string().substr(20, 4) + "0";
							if (imm[0] == '0')
							{
								state.EX.Imm = stoi(imm, 0, 2);
							}
							else if (imm[0] == '1')
							{
								state.EX.Imm = stoi(imm, 0, 2);
								state.EX.Imm |= bitset<32>("11111111111111111110000000000000");
							}
							state.IF.jump_value = state.EX.Imm;

						}
						else
						{
							state.EX.nop = true;
							//state.EX.is_I_type = true;

							state.MEM.nop = true;
							//state.MEM.rd_mem = false;
							//state.MEM.wrt_mem = false;

							state.WB.nop = true;
						}
					}
					else if (state.ID.Instr.to_string().substr(17, 3) == "001")//bne
					{
						if (!identical)
						{
							state.IF.jump = true;
							state.EX.nop = true;
							//state.EX.is_I_type = true;

							state.MEM.nop = true;
							//state.MEM.rd_mem = false;
							//state.MEM.wrt_mem = false;

							state.WB.nop = true;
							//state.WB.wrt_enable = false;

							state.EX.ALU_control = 0;

							string imm = state.ID.Instr.to_string().substr(0, 1) + state.ID.Instr.to_string().substr(24, 1) +
								state.ID.Instr.to_string().substr(1, 6) + state.ID.Instr.to_string().substr(20, 4) + "0";
							if (imm[0] == '0')
							{
								state.EX.Imm = stoi(imm, 0, 2);
							}
							else if (imm[0] == '1')
							{
								state.EX.Imm = stoi(imm, 0, 2);
								state.EX.Imm |= bitset<32>("11111111111111111110000000000000");
							}
							state.IF.jump_value = state.EX.Imm;
						}
						else
						{
							state.EX.nop = true;
							//state.EX.is_I_type = true;

							state.MEM.nop = true;
							//state.MEM.rd_mem = false;
							//state.MEM.wrt_mem = false;

							state.WB.nop = true;
						}
					}
				}
				else if (state.ID.Instr.to_string().substr(25, 7) == "0000011")//lw
				{
					state.EX.nop = false;
					state.EX.is_I_type = true;

					state.MEM.nop = false;
					state.MEM.rd_mem = true;
					state.MEM.wrt_mem = false;

					state.WB.nop = false;

					state.EX.ALU_control = 0;
					state.EX.Rs = stoi(state.ID.Instr.to_string().substr(12, 5), 0, 2);
					state.EX.Read_data1 = myRF.readRF(state.EX.Rs); //readdata1= rs value
					state.EX.Wrt_reg_addr = stoi(state.ID.Instr.to_string().substr(20, 5), 0, 2);
					state.WB.Wrt_reg_addr = state.EX.Wrt_reg_addr;
					string imm = state.ID.Instr.to_string().substr(0, 12);
					if (imm[0] == '0')
					{
						state.EX.Imm = stoi(imm, 0, 2);
					}
					else if (imm[0] == '1')
					{
						state.EX.Imm = stoi(imm, 0, 2);
						state.EX.Imm |= bitset<32>("11111111111111111111000000000000");
					}
				}
				else if (state.ID.Instr.to_string().substr(25, 7) == "0100011")//sw
				{
					state.EX.nop = false;
					state.EX.is_I_type = true;

					state.MEM.nop = false;
					state.MEM.rd_mem = false;
					state.MEM.wrt_mem = true;

					state.WB.nop = true;

					state.EX.ALU_control = 0;

					state.EX.Rs = stoi(state.ID.Instr.to_string().substr(12, 5), 0, 2);
					state.EX.Rt = stoi(state.ID.Instr.to_string().substr(7, 5), 0, 2);
					state.EX.Read_data1 = myRF.readRF(state.EX.Rs); //readdata1= rs value
					state.EX.Read_data2 = myRF.readRF(state.EX.Rt); //readdata2= rt value
					state.MEM.Store_data = state.EX.Read_data2;//MEM writedata
					string imm = state.ID.Instr.to_string().substr(0, 7) + state.ID.Instr.to_string().substr(20, 5);
					if (imm[0] == '0')
					{
						state.EX.Imm = stoi(imm, 0, 2);
					}
					else if (imm[0] == '1')
					{
						state.EX.Imm = stoi(imm, 0, 2);
						state.EX.Imm |= bitset<32>("11111111111111111111000000000000");
					}
				}

			}

			/* EX */
			if (!state.EX.nop)
			{
				if (state.EX.ALU_control == 0)//add
				{
					if (!state.EX.is_I_type)//no imm
					{
						state.MEM.ALUresult = add(state.EX.Read_data1, state.EX.Read_data2);
					}
					else//imm
					{
						state.MEM.ALUresult = add(state.EX.Read_data1, state.EX.Imm);
					}
				}
				else if (state.EX.ALU_control == 1)//sub
				{
					state.MEM.ALUresult = sub(state.EX.Read_data1, state.EX.Read_data2);
				}
				else if (state.EX.ALU_control == 2)//xor
				{
					if (!state.EX.is_I_type)//no imm
					{
						state.MEM.ALUresult = state.EX.Read_data1^state.EX.Read_data2;
					}
					else//imm
					{
						state.MEM.ALUresult = state.EX.Read_data1^state.EX.Imm;
					}
				}
				else if (state.EX.ALU_control == 3)//or
				{
					if (!state.EX.is_I_type)//no imm
					{
						state.MEM.ALUresult = state.EX.Read_data1 | state.EX.Read_data2;
					}
					else//imm
					{
						state.MEM.ALUresult = state.EX.Read_data1 | state.EX.Imm;
					}
				}
				else if (state.EX.ALU_control == 4)//and
				{
					if (!state.EX.is_I_type)//no imm
					{
						state.MEM.ALUresult = state.EX.Read_data1 & state.EX.Read_data2;;
					}
					else//imm
					{
						state.MEM.ALUresult = state.EX.Read_data1 & state.EX.Imm;
					}
				}
				state.WB.Wrt_data = state.MEM.ALUresult;
			}


			/* MEM */
			if (!state.MEM.nop)
			{
				if (state.MEM.rd_mem)
				{
					state.WB.Wrt_data = ext_dmem.readDataMem(state.MEM.ALUresult);
				}
				else if (state.MEM.wrt_mem)
				{
					ext_dmem.writeDataMem(state.MEM.ALUresult, state.MEM.Store_data);
				}
			}

			/* WB */
			if (!state.WB.nop)
			{
				myRF.writeRF(state.WB.Wrt_reg_addr, state.WB.Wrt_data);
			}

			//if (!state.IF.jump) state.IF.jump_value = 4;
			//nextState.IF.PC = add(state.IF.PC, state.IF.jump_value);
		}

		if (!state.IF.nop)
		{
			if (!state.IF.jump) state.IF.jump_value = 4;
			nextState.IF.PC = add(state.IF.PC, state.IF.jump_value);
			state = nextState; //The end of the cycleand updates the current state with the values calculated in this cycle
		}
		else nextState = state;

		myRF.outputRF(cycle); // dump RF
		printState(nextState, cycle); //print states after executing cycle 0, cycle 1, cycle 2 ... 

		cycle++;
	}

	void printState(stateStruct state, int cycle) {
		ofstream printstate;
		if (cycle == 0)
			printstate.open(opFilePath, std::ios_base::trunc);
		else
			printstate.open(opFilePath, std::ios_base::app);
		if (printstate.is_open()) {
			printstate << "State after executing cycle:\t" << cycle << endl;

			printstate << "IF.PC:\t" << state.IF.PC.to_ulong() << endl;
			printstate << "IF.nop:\t" << state.IF.nop << endl;
		}
		else cout << "Unable to open SS StateResult output file." << endl;
		printstate.close();
	}
private:
	string opFilePath;
};

//performance
void performance(string ioDir, int cycle, int instrNum) {
	ofstream printPerformance;
	string performanceFile = ioDir + "\\PerformanceMetrics_Result_SS.txt";
	printPerformance.open(performanceFile, std::ios::trunc);

	printPerformance << "\n \r" << endl;
	printPerformance << "Single Stage Core Performance Metrics-----------------------------" << endl;
	printPerformance << "Number of cycles taken: " << cycle << endl;
	printPerformance << "Cycles per instruction: " << float(cycle) / instrNum << endl;
	printPerformance << "Instructions per cycle: " << float(instrNum) / cycle << endl;
	printPerformance << "\n \r" << endl;
}

int main(int argc, char* argv[]) {

	//string ioDir = "C:\Users\Li Yuhan\Desktop\test";

	string ioDir = "";
	
	if (argc == 1) {
		cout << "Enter path containing the memory files: ";
		getline(cin, ioDir);
		//cin >> ioDir;
	}
	else if (argc > 2) {
		cout << "Invalid number of arguments. Machine stopped." << endl;
		return -1;
	}
	else {
		ioDir = argv[1];
		cout << "IO Directory: " << ioDir << endl;
	}
	

	InsMem imem = InsMem("Imem", ioDir);
	DataMem dmem_ss = DataMem("SS", ioDir);
	//DataMem dmem_fs = DataMem("FS", ioDir);

	SingleStageCore SSCore(ioDir, imem, dmem_ss);
	//FiveStageCore FSCore(ioDir, imem, dmem_fs);

	while (1) {
		if (!SSCore.halted)
			SSCore.step();
		else break;
	}

	//performance
	performance(ioDir, SSCore.cycle, imem.i / 4);

	// dump SS and FS data mem.
	SSCore.ext_dmem.outputDataMem();
	return 0;
}