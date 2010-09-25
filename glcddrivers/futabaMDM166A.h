/*
 * GraphLCD driver library
 *
 * (C) 2010 Andreas Brachold <vdr07 AT deltab de>
 *
 * This file is released under the GNU General Public License. 
 *
 * See the files README and COPYING for details.
 *
 */

#ifndef _GLCDDRIVERS_FutabaMDM166A_H_
#define _GLCDDRIVERS_FutabaMDM166A_H_

#include "driver.h"

#define HAVE_STDBOOL_H
#include <hid.h>
#include <queue>

namespace GLCD
{
	class cDriverConfig;

	class cHIDQueue : public std::queue<byte> {
			HIDInterface* hid;
			bool bInit;
		public:
			cHIDQueue();
			virtual ~cHIDQueue();
			virtual bool open();
			virtual void close();
			virtual bool isopen() const { return hid != 0; }
			void Cmd(const byte & cmd);
			void Data(const byte & data);
			bool Flush();
		private:
			const char *hiderror(hid_return ret) const;
	};

	class cDriverFutabaMDM166A : public cDriver, cHIDQueue 
	{
		cDriverConfig * config;
		cDriverConfig * oldConfig;
		unsigned char *m_pDrawMem; // the draw "memory"
		unsigned char *m_pVFDMem;  // the double buffed display "memory"
		unsigned int m_iSizeYb;
		unsigned int m_nRefreshCounter;
		unsigned int lastIconState;
		int CheckSetup();
	protected:
		void ClearVFDMem();
		void SetPixel(int x, int y);
		void icons(unsigned int state);
		bool SendCmdClock();
		bool SendCmdShutdown();

	public:
		cDriverFutabaMDM166A(cDriverConfig * config);
		virtual ~cDriverFutabaMDM166A();

		virtual int Init();
		virtual int DeInit();

		virtual void Clear();
		virtual void Set8Pixels(int x, int y, byte data);
		virtual void Refresh(bool refreshAll = false);

		virtual void SetBrightness(unsigned int percent);
	};
};
#endif



