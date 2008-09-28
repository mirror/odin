/******************************************************************************

    ODIN - Open Disk Imager in a Nutshell

    Copyright (C) 2008

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>

    For more information and the latest version of the source code see
    <http://sourceforge.net/projects/odin-win>

******************************************************************************/
 
///////////////////////////////////////////////////////////////////////////////////////////
// class CRC32 a c++ class for calculation of crc32 checksums
///////////////////////////////////////////////////////////////////////////////////////////


class CCRC32
{
public:
  CCRC32();
  ~CCRC32();
  void AddDataBlock(BYTE* pData, unsigned length);
  DWORD GetResult();

private:
  inline void CalcCRC32(const BYTE byte);
  void Init();
  void Reset();

  DWORD fCrc32;
  static const DWORD sCrc32Table[256];
};
