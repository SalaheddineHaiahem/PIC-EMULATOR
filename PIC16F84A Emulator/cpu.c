//
//  cpu.c
//  PIC16F84A Emulator
//
//  Created by Cameron Gutman on 11/13/12.
//  Copyright (c) 2012 Cameron Gutman. All rights reserved.
//

#include <stdio.h>

#include "cpu.h"
#include "opcode.h"

int CpuInitializeCore(PIC_CPU *Cpu)
{
    //Initialize register file
    RegsInitializeRegisterFile(&Cpu->Regs);

    //W and SRAM state is left undefined

    //Success
    return 0;
}

int CpuInitializeProgramMemory(PIC_CPU *Cpu, unsigned char *buffer, int size)
{
    //Make sure the bytecode fits in the PIC's memory
    if (size > PROGRAM_MEM_SIZE)
    {
        printf("Program is too large for the PIC\n");
        return -1;
    }

    //Copy the bytecode into our private memory
    memcpy(Cpu->ProgMem, buffer, size);

    //Success
    return 0;
}

int CpuExecuteOpcode(PIC_CPU *Cpu, short opcode, unsigned short PC)
{
    unsigned char result;
    unsigned char op1, op2;
    unsigned char status = Cpu->Regs.STATUS;
    unsigned char oldW = Cpu->W;
    unsigned char oldStatus = status;

    //Make sure this is 14-bit
    if ((opcode & 0xC000) != 0)
    {
        printf("Invalid high bits in opcode");
        return -1;
    }
    
    //Classify the opcode by the highest byte
    if ((opcode & 0x3000) == 0x0000)
    {
        //Byte-oriented file register operations
        switch (opcode & 0xF00)
        {
            case OP_ADDWF:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x80) >> 7;

                //If the low 4-bit add overflowed, set the DC bit
                result = (Cpu->W & 0x0F) + (RegsGetValue(&Cpu->Regs, op1) & 0x0F);
                if ((result & 0x10) != 0)
                    status |= STATUS_DC;
                else
                    status &= ~STATUS_DC;
                
                //If the high 4-bit add overflowed, set the C bit
                result = (Cpu->W >> 4) + (RegsGetValue(&Cpu->Regs, op1) >> 4);
                if ((result & 0x10) != 0)
                    status |= STATUS_C;
                else
                    status &= ~STATUS_C;
                
                //Do the operation
                result = Cpu->W + RegsGetValue(&Cpu->Regs, op1);
                
                //Store the results
                if (op2 == 0)
                    Cpu->W = result;
                else
                    RegsSetValue(&Cpu->Regs, op1, result);
                
                //Set status flags
                if (result == 0)
                    status |= STATUS_Z;
                else
                    status &= ~STATUS_Z;
                break;
                
            case OP_ANDWF:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x80) >> 7;
                
                //Execute the operation
                result = Cpu->W & RegsGetValue(&Cpu->Regs, op1);
                
                //Store the results
                if (op2 == 0)
                    Cpu->W = result;
                else
                    RegsSetValue(&Cpu->Regs, op1, result);
                
                //Set status flags
                if (result == 0)
                    status |= STATUS_Z;
                else
                    status &= ~STATUS_Z;

                break;
        
            case 0x100:
                if ((opcode & 0x80) != 0)
                {
                    //CLRF

                    //Get the operand
                    op1 = (opcode & 0x7F);
                    
                    //Execute the operation
                    RegsSetValue(&Cpu->Regs, op1, 0);
                    
                    //Set status flags
                    status |= STATUS_Z;
                }
                else
                {
                    //CLRW
                    
                    //Execute the operation
                    Cpu->W = 0;

                    //Set status flags
                    status |= STATUS_Z;
                }
                break;

            case OP_COMF:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x80) >> 7;
                
                //Execute the operation
                result = ~ RegsGetValue(&Cpu->Regs, op1);
                
                //Store the results
                if (op2 == 0)
                    Cpu->W = result;
                else
                    RegsSetValue(&Cpu->Regs, op1, result);
                
                //Set status flags
                if (result == 0)
                    status |= STATUS_Z;
                else
                    status &= ~STATUS_Z;
                
                break;
                
            case OP_DECF:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x80) >> 7;
                
                //Execute the operation
                result = RegsGetValue(&Cpu->Regs, op1) - 1;
                
                //Store the results
                if (op2 == 0)
                    Cpu->W = result;
                else
                    RegsSetValue(&Cpu->Regs, op1, result);
                
                //Set status flags
                if (result == 0)
                    status |= STATUS_Z;
                else
                    status &= ~STATUS_Z;
                
                break;
            case OP_DECFSZ:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x80) >> 7;
                
                //Execute the operation
                result = RegsGetValue(&Cpu->Regs, op1) - 1;
                
                //Store the results
                if (op2 == 0)
                    Cpu->W = result;
                else
                    RegsSetValue(&Cpu->Regs, op1, result);
                
                //Perform operation if 0
                if (result == 0)
                    PC += PIC_OPCODE_SIZE;
                
                break;

            case OP_INCF:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x80) >> 7;
                
                //Execute the operation
                result = RegsGetValue(&Cpu->Regs, op1) + 1;
                
                //Store the results
                if (op2 == 0)
                    Cpu->W = result;
                else
                    RegsSetValue(&Cpu->Regs, op1, result);
                
                //Set status flags
                if (result == 0)
                    status |= STATUS_Z;
                else
                    status &= ~STATUS_Z;
                
                break;
                
            case OP_INCFSZ:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x80) >> 7;
                
                //Execute the operation
                result = RegsGetValue(&Cpu->Regs, op1) + 1;
                
                //Store the results
                if (op2 == 0)
                    Cpu->W = result;
                else
                    RegsSetValue(&Cpu->Regs, op1, result);
                
                //Perform operation if 0
                if (result == 0)
                    PC += PIC_OPCODE_SIZE;
                break;
                
            case OP_IORWF:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x80) >> 7;
                
                //Execute the operation
                result = Cpu->W | RegsGetValue(&Cpu->Regs, op1);
                
                //Store the results
                if (op2 == 0)
                    Cpu->W = result;
                else
                    RegsSetValue(&Cpu->Regs, op1, result);
                
                //Set status flags
                if (result == 0)
                    status |= STATUS_Z;
                else
                    status &= ~STATUS_Z;
                break;
                
            case OP_MOVF:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x80) >> 7;
                
                //Execute the operation
                result = RegsGetValue(&Cpu->Regs, op1);
                
                //Store the results
                if (op2 == 0)
                    Cpu->W = result;
                else
                    RegsSetValue(&Cpu->Regs, op1, result);
                
                //Set status flags
                if (result == 0)
                    status |= STATUS_Z;
                else
                    status &= ~STATUS_Z;
                break;

            case 0x000:
                if ((opcode & 0x80) != 0)
                {
                    //MOVWF

                    //Get the operand
                    op1 = (opcode & 0x7F);
                    
                    //Execute the operation
                    RegsSetValue(&Cpu->Regs, op1, Cpu->W);
                }
                else if ((opcode & 0xF) == 0)
                {
                    //Nop
                }
                else
                {
                    //Control ops
                    switch (opcode & 0xFF)
                    {
                        case OP_CLRWDT:
                            printf("CLRWDT");
                            break;
                        case OP_RETFIE:
                            printf("RETFIE");
                            break;
                        case OP_RETURN:
                            printf("RETURN");
                            break;
                        case OP_SLEEP:
                            printf("SLEEP");
                            break;
                        default:
                            printf("Invalid 00 0000 opcode!");
                            break;
                    }
                    return -1;
                }
                break;
                
            case OP_RLF:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x80) >> 7;
                
                //Execute the operation
                result = RegsGetValue(&Cpu->Regs, op1);
                
                //Bit 7 -> Carry bit
                if ((result & 0x80) != 0)
                    status |= STATUS_C;
                else
                    status &= ~STATUS_C;

                //Shift left 1
                result = result << 1;
                
                //Carry bit -> Bit 0
                if ((Cpu->Regs.STATUS & STATUS_C) != 0)
                    result |= 0x01;
                else
                    result &= ~0x01;
                
                //Store the results
                if (op2 == 0)
                    Cpu->W = result;
                else
                    RegsSetValue(&Cpu->Regs, op1, result);

                break;
                
            case OP_RRF:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x80) >> 7;
                
                //Execute the operation
                result = RegsGetValue(&Cpu->Regs, op1);
                
                //Bit 0 -> Carry bit
                if (result & 0x01)
                    status |= STATUS_C;
                else
                    status &= ~STATUS_C;
                
                //Shift right 1
                result = result >> 1;
                
                //Carry bit -> Bit 7
                if ((Cpu->Regs.STATUS & STATUS_C) != 0)
                    result |= 0x80;
                else
                    result &= ~0x80;
                
                //Store the results
                if (op2 == 0)
                    Cpu->W = result;
                else
                    RegsSetValue(&Cpu->Regs, op1, result);

                break;
                
            case OP_SUBWF:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x80) >> 7;
                
                //If the low 4-bit sub underflowed, set the DC bit
                result = (RegsGetValue(&Cpu->Regs, op1) & 0x0F) - (Cpu->W & 0x0F);
                
                //Polarity is reversed for SUB
                if ((result & 0x10) == 0)
                    status |= STATUS_DC;
                else
                    status &= ~STATUS_DC;
                
                //If the high 4-bit sub underflowed, set the C bit
                result = (RegsGetValue(&Cpu->Regs, op1) >> 4) - (Cpu->W >> 4);
                
                //Polarity is reversed for SUB
                if ((result & 0x10) == 0)
                    status |= STATUS_C;
                else
                    status &= ~STATUS_C;
                
                //Execute the operation
                result = RegsGetValue(&Cpu->Regs, op1) - Cpu->W;
                
                //Store the results
                if (op2 == 0)
                    Cpu->W = result;
                else
                    RegsSetValue(&Cpu->Regs, op1, result);
                
                //Set status flags
                if (result == 0)
                    status |= STATUS_Z;
                else
                    status &= ~STATUS_Z;
                
                break;

            case OP_SWAPF:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x80) >> 7;
                
                //Execute the operation
                result = (RegsGetValue(&Cpu->Regs, op1) << 4) & 0xF0;
                result |= (RegsGetValue(&Cpu->Regs, op1) >> 4) & 0x0F;
                
                //Store the results
                if (op2 == 0)
                    Cpu->W = result;
                else
                    RegsSetValue(&Cpu->Regs, op1, result);
                
                break;
                
            case OP_XORWF:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x80) >> 7;
                
                //Execute the operation
                result = Cpu->W ^ RegsGetValue(&Cpu->Regs, op1);
                
                //Store the results
                if (op2 == 0)
                    Cpu->W = result;
                else
                    RegsSetValue(&Cpu->Regs, op1, result);
                
                //Set status flags
                if (result == 0)
                    status |= STATUS_Z;
                else
                    status &= ~STATUS_Z;
                break;
            default:
                printf("Invalid 00 opcode!");
                return -1;
        }
    }
    else if ((opcode & 0x3000) == 0x1000)
    {
        //Bit-oriented file register operations
        switch (opcode & 0x3C00)
        {
            case OP_BCF:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x380) >> 7;
                
                //Execute the operation
                result = RegsGetValue(&Cpu->Regs, op1) & ~(1 << op2);
                
                //Store the results
                RegsSetValue(&Cpu->Regs, op1, result);
                break;
                
            case OP_BSF:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x380) >> 7;
                
                //Execute the operation
                result = RegsGetValue(&Cpu->Regs, op1) | (1 << op2);
                
                //Store the results
                RegsSetValue(&Cpu->Regs, op1, result);
                break;
                
            case OP_BTFSC:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x380) >> 7;
                
                //Execute the operation
                result = RegsGetValue(&Cpu->Regs, op1) & (1 << op2);
                
                //Perform the operation
                if (result == 0)
                    PC += PIC_OPCODE_SIZE;
                break;
                
            case OP_BTFSS:
                //Get the operands
                op1 = (opcode & 0x7F);
                op2 = (opcode & 0x380) >> 7;
                
                //Execute the operation
                result = RegsGetValue(&Cpu->Regs, op1) & (1 << op2);
                
                //Perform the operation
                if (result != 0)
                    PC += PIC_OPCODE_SIZE;
                break;
                
            default:
                printf("Invalid 01 opcode!");
                return -1;
        }
    }
    else if ((opcode & 0x3000) == 0x2000)
    {
        //GOTO / CALL
        if ((opcode & 0x800) != 0)
        {
            printf("GOTO %d", (opcode & 0x7FF));
        }
        else
        {
            printf("CALL %d", (opcode & 0x7FF));
        }
        return -1;
    }
    else //0x3000
    {
        if ((opcode & 0xE00) == 0xE00)
        {
            //ADDLW
            
            //Get the operand
            op1 = (opcode & 0xFF);

            //If the low 4-bit add overflowed, set the DC bit
            result = (Cpu->W & 0x0F) + (op1 & 0x0F);
            if ((result & 0x10) != 0)
                status |= STATUS_DC;
            else
                status &= ~STATUS_DC;
                        
            //If the high 4-bit add overflowed, set the C bit
            result = (Cpu->W >> 4) + (op1 >> 4);
            if ((result & 0x10) != 0)
                status |= STATUS_C;
            else
                status &= ~STATUS_C;

            //Do the operation
            result = Cpu->W + op1;
            
            //Store the results
            Cpu->W = result;
            
            //Set status flags
            if (result == 0)
                status |= STATUS_Z;
            else
                status &= ~STATUS_Z;
        }
        else if ((opcode & 0xF00) == 0x900)
        {
            //ANDLW
            
            //Get the operand
            op1 = (opcode & 0xFF);
            
            //Execute the operation
            result = Cpu->W & op1;
            
            //Store the results
            Cpu->W = result;
            
            //Set status flags
            if (result == 0)
                status |= STATUS_Z;
            else
                status &= ~STATUS_Z;
        }
        else if ((opcode & 0xF00) == 0x800)
        {
            //IORLW
            
            //Get the operand
            op1 = (opcode & 0xFF);
            
            //Execute the operation
            result = Cpu->W | op1;
            
            //Store the results
            Cpu->W = result;
            
            //Set status flags
            if (result == 0)
                status |= STATUS_Z;
            else
                status &= ~STATUS_Z;
        }
        else if ((opcode & 0xF00) == 0xA00)
        {
            //XORLW
            
            //Get the operand
            op1 = (opcode & 0xFF);
            
            //Execute the operation
            result = Cpu->W ^ op1;
            
            //Store the results
            Cpu->W = result;
            
            //Set status flags
            if (result == 0)
                status |= STATUS_Z;
            else
                status &= ~STATUS_Z;
        }
        else if ((opcode & 0xC00) == 0x000)
        {
            //MOVLW
            
            //Get the operand
            op1 = (opcode & 0xFF);
            
            //Execute the operation
            result = op1;
            
            //Store the results
            Cpu->W = result;
        }
        else if ((opcode & 0xC00) == 0x400)
        {
            printf("RETLW %d", (opcode & 0xFF));
            return -1;
        }
        else if ((opcode & 0xC00) == 0xC00)
        {
            //SUBLW
            
            //Get the operand
            op1 = (opcode & 0xFF);
            
            //If the low 4-bit sub underflowed, set the DC bit
            result = (op1 & 0x0F) - (Cpu->W & 0x0F);
            
            //Polarity is reversed for SUB
            if ((result & 0x10) == 0)
                status |= STATUS_DC;
            else
                status &= ~STATUS_DC;
            
            //If the high 4-bit sub underflowed, set the C bit
            result = (op1 >> 4) - (Cpu->W >> 4);
            
            //Polarity is reversed for SUB
            if ((result & 0x10) == 0)
                status |= STATUS_C;
            else
                status &= ~STATUS_C;
            
            //Execute the operation
            result = op1 - Cpu->W;
            
            //Store the results
            Cpu->W = result;
            
            //Set status flags
            if (result == 0)
                status |= STATUS_Z;
            else
                status &= ~STATUS_Z;
        }
        else
        {
            printf("Invalid 11 opcode!");
            return -1;
        }
    }
    
    if (status != oldStatus)
    {
        printf("STATUS:[");
        RegsPrintStatusRegister(Cpu->Regs.STATUS);
        printf("] -> [");
        Cpu->Regs.STATUS = status;
        RegsPrintStatusRegister(Cpu->Regs.STATUS);
        printf("]\n");
    }

    if (oldW != Cpu->W)
    {
        printf("W: %d -> %d\n", oldW, Cpu->W);
    }

    return PC;
}

//Executes one instruction
int CpuExec(PIC_CPU *Cpu)
{
    short PC;
    short opcode;

    //Construct the full PC and do wrap-around handling
    PC = ((Cpu->Regs.PCLATH << 8) | Cpu->Regs.PCL) & (PROGRAM_MEM_SIZE - 1);

    //Grab the 14-bit opcode from the program memory
    opcode = (*((short*)&Cpu->ProgMem[PC])) & PIC_OPCODE_MASK;

    //Exe
    
    //Increment the PC and write it back
    PC = PC + PIC_OPCODE_SIZE;
    Cpu->Regs.PCLATH = (PC << 8) & 0x1F;
    Cpu->Regs.PCL = (PC & 0xFF);

    //1 instruction retired :)
    return 0;
}