#include "stdafx.h"
#include "BaseMapper.h"

class MMC3 : public BaseMapper
{
	private: 
		enum class MMC3Registers
		{
			Reg8000 = 0x8000,
			Reg8001 = 0x8001,
			RegA000 = 0xA000,
			RegA001 = 0xA001,
			RegC000 = 0xC000,
			RegC001 = 0xC001,
			RegE000 = 0xE000,
			RegE001 = 0xE001
		};

		uint8_t _currentRegister;
		uint8_t _registers[8];
		uint8_t _chrMode;
		uint8_t _prgMode;

		uint8_t _irqReloadValue;
		uint8_t _irqCounter;
		bool _irqReload;

		bool _irqEnabled;
		int32_t _lastPPUCycle;

		bool _wramEnabled;
		bool _wramWriteProtected;

		struct {
			uint8_t Reg8000;
			uint8_t RegA000;
			uint8_t RegA001;
		} _state;

		void Reset()
		{
			_state.Reg8000 = 0;
			_state.RegA000 = 0;
			_state.RegA001 = 0;
			_chrMode = 0;
			_prgMode = 0;
			_currentRegister = 0;
			memset(_registers, 0, sizeof(_registers));

			_irqCounter = 0;
			_irqReloadValue = 0;
			_irqReload = false;
			_irqEnabled = false;
			_lastPPUCycle = 0xFFFF;

			_wramEnabled = false;
			_wramWriteProtected = false;
		}

		void UpdateState()
		{
			_currentRegister = _state.Reg8000 & 0x07;
			_chrMode = (_state.Reg8000 & 0x80) >> 7;
			_prgMode = (_state.Reg8000 & 0x40) >> 6;

			if(_mirroringType != MirroringType::FourScreens) {
				_mirroringType = ((_state.RegA000 & 0x01) == 0x01) ? MirroringType::Horizontal : MirroringType::Vertical;
			}

			_wramEnabled = (_state.RegA001 & 0x80) == 0x80;
			_wramWriteProtected = (_state.RegA001 & 0x40) == 0x40;

			if(_prgMode == 0) {
				SelectPRGPage(0, _registers[6]);
				SelectPRGPage(1, _registers[7]);
				SelectPRGPage(2, -2);
				SelectPRGPage(3, -1);
			} else if(_prgMode == 1) {
				SelectPRGPage(0, -2);
				SelectPRGPage(1, _registers[7]);
				SelectPRGPage(2, _registers[6]);
				SelectPRGPage(3, -1);
			}

			if(_chrMode == 0) {
				SelectCHRPage(0, _registers[0]);
				SelectCHRPage(1, _registers[0]+1);
				SelectCHRPage(2, _registers[1]);
				SelectCHRPage(3, _registers[1]+1);

				SelectCHRPage(4, _registers[2]);
				SelectCHRPage(5, _registers[3]);
				SelectCHRPage(6, _registers[4]);
				SelectCHRPage(7, _registers[5]);
			} else if(_chrMode == 1) {
				SelectCHRPage(0, _registers[2]);
				SelectCHRPage(1, _registers[3]);
				SelectCHRPage(2, _registers[4]);
				SelectCHRPage(3, _registers[5]);

				SelectCHRPage(4, _registers[0]);
				SelectCHRPage(5, _registers[0]+1);
				SelectCHRPage(6, _registers[1]);
				SelectCHRPage(7, _registers[1]+1);
			}

		}

	protected:
		virtual uint32_t GetPRGPageSize() { return 0x2000; }
		virtual uint32_t GetCHRPageSize() {	return 0x0400; }

		void InitMapper() 
		{
			Reset();
			UpdateState();
		}

	public:
		void WriteRAM(uint16_t addr, uint8_t value)
		{
			switch((MMC3Registers)(addr & 0xE001)) {
				case MMC3Registers::Reg8000:
					_state.Reg8000 = value;
					UpdateState();
					break;

				case MMC3Registers::Reg8001:
					if(_currentRegister >= 6) {
						//"Writes to registers 6 and 7 always ignore bits 6 and 7, as the MMC3 has only 6 PRG ROM address output lines."
						value &= 0x3F;
					} else if(_currentRegister <= 1) {
						//"Writes to registers 0 and 1 always ignore bit 0"
						value &= ~0x01;
					}
					_registers[_currentRegister] = value;
					UpdateState();
					break;

				case MMC3Registers::RegA000:
					_state.RegA000 = value;
					UpdateState();
					break;

				case MMC3Registers::RegA001:
					_state.RegA001 = value;
					UpdateState();
					break;

				case MMC3Registers::RegC000:
					_irqReloadValue = value;
					break;

				case MMC3Registers::RegC001:
					_irqCounter = 0;
					_irqReload = true;
					break;

				case MMC3Registers::RegE000:
					_irqEnabled = false;
					//Remove SetIRQ flag if it was added to cpu?
					break;

				case MMC3Registers::RegE001:
					_irqEnabled = true;
					break;
			}
		}

		uint32_t _a12Counter = 0;
		uint32_t _lastCycle = 0xFFFF;

		uint8_t ReadVRAM(uint16_t addr)
		{
			/*uint32_t cycle = PPU::GetCurrentCycle();
			if((_lastCycle - cycle >= 8 || cycle < _lastCycle) && (addr & 0x1000)) {
				uint32_t count = _irqCounter;
				if(_irqCounter == 0 || _irqReload) {
					_irqCounter = _irqReloadValue;
				} else {
					_irqCounter--;
				}
				if((count > 0 || _irqReload) && _irqCounter == 0 && _irqEnabled) {
					CPU::SetIRQFlag();
				}
				_irqReload = false;

				_lastCycle = cycle;
			}*/
			return BaseMapper::ReadVRAM(addr);
		}
};