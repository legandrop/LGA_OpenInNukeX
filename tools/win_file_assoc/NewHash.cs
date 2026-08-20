using System.Reflection;
using Microsoft.Win32;
using Microsoft.Win32.SafeHandles;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace DefaultApps
{
    static public class NewHash
    {
        static bool Ready = false;

        static IntPtr OpenWithLibraryHandle = IntPtr.Zero;

        static NewHash()
        {
            if (File.Exists("C:\\Windows\\System32\\Windows.Internal.OpenWithHost.dll") == false)
                return;

            OpenWithLibraryHandle = NativeMethods.LoadLibrary("C:\\Windows\\System32\\Windows.Internal.OpenWithHost.dll");

            using var fs = new FileStream("C:\\Windows\\System32\\Windows.Internal.OpenWithHost.dll", FileMode.Open, FileAccess.Read);
            using var br = new BinaryReader(fs, Encoding.Unicode);

            while (fs.Position < fs.Length)
            {
                var prevPos = fs.Position;
                if (PatternMatch(br,HashValidationSearchPattern))
                {
                    fs.Position = prevPos;

                    var chars = br.ReadChars(HashValidationStringSearchLength);
                    var hashValidationString = new string(chars);
                    var stringLower = hashValidationString.ToLowerInvariant();
                    int hashIndex = 0;
                    foreach (var hash in HashValidationStringHashes)
                    {
                        if (stringLower.EndsWith(hash.Key))
                        {
                            var stringHash = SFTA.md5.ComputeHash(Encoding.Unicode.GetBytes(stringLower));
                            if (Convert.ToHexStringLower(stringHash) == hash.Value)
                            {
                                HashValidationStrings[hashIndex] = hashValidationString;
                                break;
                            }
                        }
                        hashIndex++;
                    }
                }

                fs.Position = prevPos;

                if (PatternMatch(br, FuncProloguePattern))
                {
                    var pos = fs.Position;

                    if(PatternMatch(br, CalculateHashStringPattern))
                    {
                        CalculateHashString = Marshal.GetDelegateForFunctionPointer<CalculateHashStringFunc>(new IntPtr(OpenWithLibraryHandle + prevPos));
                    }

                    fs.Position = pos;

                    if(PatternMatch(br, HashThing1Pattern) == false)
                    {
                        fs.Position = prevPos + 1;
                        continue;
                    }

                    HashThing1 = Marshal.GetDelegateForFunctionPointer<HashThing1Func>(new IntPtr(OpenWithLibraryHandle + prevPos));
                }

                fs.Position = prevPos + 1;
            }

            Ready = true;

            for(int i = 0; i < HashValidationStrings.Length; i++)
            {
                if (string.IsNullOrEmpty(HashValidationStrings[i]))
                {
                    Ready = false;
                    break;
                }
            }
        }

        public static bool PatternMatch(BinaryReader br, byte[] patternBytes)
        {
            for (int i = 0; i < patternBytes.Length; i++)
            {
                if (br.ReadByte() != patternBytes[i])
                {
                    return false;
                }
            }
            return true;
        }

        #region Lookup Tables

        static byte[] LookUpLut1 =  // TODO: Convert to a math function 0 1 2 4 8 16 seems pretty straightforward
            [0, 1, 2, 2, 3, 3, 3, 3,
             4, 4, 4, 4, 4, 4, 4, 4,
             5, 5, 5, 5, 5, 5, 5, 5,
             5, 5, 5, 5, 5, 5, 5, 5,
             6, 6, 6, 6, 6, 6, 6, 6,
             6, 6, 6, 6, 6, 6, 6, 6,
             6, 6, 6, 6, 6, 6, 6, 6,
             6, 6, 6, 6, 6, 6, 6, 6,
             7, 7, 7, 7, 7, 7, 7, 7,
             7, 7, 7, 7, 7, 7, 7, 7,
             7, 7, 7, 7, 7, 7, 7, 7,
             7, 7, 7, 7, 7, 7, 7, 7,
             7, 7, 7, 7, 7, 7, 7, 7,
             7, 7, 7, 7, 7, 7, 7, 7,
             7, 7, 7, 7, 7, 7, 7, 7,
             7, 7, 7, 7, 7, 7, 7, 7,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8,
             8, 8, 8, 8, 8, 8, 8, 8];

        static byte[] LookUpLutFuck2 =
            [0x00, 0x02, 0x06, 0x07, 0x0A, 0x18, 0x19, 0x1D,
             0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D,
             0x1D, 0x1D, 0xF5, 0xF5, 0xFA, 0xFA, 0xFA, 0xFA,
             0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFB, 0xFB, 0xFB,
             0xFB ]; // +1 because 8 + 24

        static byte[] LookUpLut3 =
            [0xA8, 0x70, 0x6F, 0x53, 0x38, 0x20, 0x1C, 0x13,
             0x13, 0x12, 0x12, 0x12, 0x10, 0x0F, 0x0F, 0x0E,
             0x0E, 0x0E, 0x0E, 0x0D, 0x0D, 0x0C, 0x0C, 0x0B,
             0x0B, 0x0B, 0x0B, 0x0A, 0x0A, 0x0A, 0x09, 0x09,
             0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
             0x09, 0x09, 0x09, 0x08, 0x08, 0x08, 0x08, 0x08,
             0x08, 0x08, 0x08, 0x08, 0x08, 0x07, 0x07, 0x07,
             0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
             0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
             0x07, 0x07, 0x07, 0x07, 0x06, 0x06, 0x06, 0x06,
             0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
             0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
             0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
             0x06, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05,
             0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
             0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
             0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
             0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
             0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
             0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
             0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
             0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
             0x05, 0x05, 0x05, 0x05, 0x04, 0x04, 0x04, 0x04,
             0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
             0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
             0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
             0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
             0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
             0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
             0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
             0x04, 0x04, 0x04, 0x04, 0x04, 0x03, 0x03, 0x03,
             0x03, 0x03, 0x02, 0x01 ];

        static byte[] LookUpLut4 = LoadLookUpLut4();

        static byte[] LoadLookUpLut4()
        {
            var baseDir = AppContext.BaseDirectory;
            var path = Path.Combine(baseDir, "LookUpLut4.bin");
            if (!File.Exists(path))
            {
                path = Path.Combine(baseDir, "Resources", "LookUpLut4.bin");
            }
            return File.ReadAllBytes(path);
        }

        static byte[] LookUpLut5 =
            [0x00, 0xFB, 0xFA, 0xF5, 0xB4, 0x6D, 0x4C, 0x35,
             0x2B, 0x1E, 0x1B, 0x17, 0x15, 0x13, 0x0F, 0x0D,
             0x0C, 0x0C, 0x09, 0x07, 0x07, 0x07, 0x07, 0x07,
             0x07, 0x07, 0x07, 0x07, 0x06, 0x06, 0x06, 0x06,
             0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
             0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
             0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
             0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
             0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
             0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
             0x04, 0x04, 0x04, 0x03, 0x03, 0x03, 0x03, 0x03,
             0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
             0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
             0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x02,
             0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
             0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
             0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
             0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
             0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
             0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
             0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];

        #endregion

        #region New Hashing Algorithm

        public static byte[] CalculateHash(string hashedString)
        {
            var hashInBytes = new uint[hashedString.Length];
            for (int i = 0; i < hashedString.Length; i++)
            {
                hashInBytes[i] = hashedString[i];
            }

            var hashLength = hashedString.Length;

            byte[] hashingBytes = new byte[(hashedString.Length + 1) * 16];

            var hashLength2 = hashLength;


            var teeee = ((hashLength + 1) / 8);

            var halfLength = teeee + 1;

            var beginBytes = new byte[4];
            var hashLengthBytes = BitConverter.GetBytes(hashLength);
            var halfLengthBytes = BitConverter.GetBytes(halfLength);
            beginBytes[0] = hashLengthBytes[0];
            beginBytes[1] = hashLengthBytes[1];
            beginBytes[2] = halfLengthBytes[0];
            beginBytes[3] = halfLengthBytes[1];

            var roundLength = ((hashLength + 1) / 8) * 8;

            hashLength2 = roundLength;


            int indexIdkHelp = 0;

            int indexGrr = 0;

            {
                do
                {
                    var hashInByte = hashInBytes[indexIdkHelp];
                    var thirdByte = hashInByte >> 16;
                    uint hashHelp3 = 0;
                    if (thirdByte == 0)
                    {
                        var secondByte = hashInByte >> 8;
                        if (secondByte == 0)
                        {
                            hashHelp3 = LookUpLut1[hashInByte]; // Will never be more than 256 because of the second if check
                        }
                        else
                        {
                            hashHelp3 = (uint)LookUpLut1[secondByte] + 8; // Will never be more than 256 because of the first if check
                        }
                    }
                    else
                    {
                        var fourthByte = hashInByte >> 24;
                        if (fourthByte == 0)
                        {
                            hashHelp3 = (uint)LookUpLut1[thirdByte] + 16; // Will never be more than 256 because y'know
                        }
                        else
                        {
                            hashHelp3 = (uint)LookUpLut1[fourthByte] + 24; // Will never be more than 256
                        }
                    }
                    long uVar5 = LookUpLutFuck2[hashHelp3];
                    hashHelp3 = LookUpLut3[uVar5];

                    if (hashLength2 < hashHelp3)
                    {
                        int hashHelp2 = hashLength - indexIdkHelp;
                        int uVar3 = 0;
                        while (indexIdkHelp < hashLength)
                        {
                            hashHelp2 = LookUpLut5[hashHelp2];
                            long lVar9 = hashHelp2 * 0xa8;

                        SecondLoop:
                            uVar5 = 0;
                            do
                            {
                                hashHelp3 = LookUpLut3[hashHelp2];
                                var shiftBy = LookUpLut4[lVar9 + uVar5];
                                var shifted = 1L << shiftBy;
                                var comp = hashInBytes[uVar5 + indexIdkHelp];
                                if (shifted <= comp)
                                {
                                    break;
                                }
                                uVar3 = (int)uVar5 + 1;
                                uVar5 = (uint)uVar3;
                            } while (uVar3 < hashHelp3);
                            if (uVar5 != hashHelp3)
                            {
                                do
                                {
                                    hashHelp2 = hashHelp2 + 1;
                                    lVar9 = (long)hashHelp2 * 0xa8;
                                    var shiftBy = LookUpLut4[lVar9 + uVar5];
                                    var shifted = 1L << shiftBy;
                                    var comp = hashInBytes[uVar5 + indexIdkHelp];
                                    if (comp < shifted)
                                    {
                                        break;
                                    }
                                } while (uVar5 < LookUpLut3[hashHelp2]);
                                goto SecondLoop;
                            }
                            lVar9 = 0;
                            int bVar6 = 0x38;
                            uVar5 = 0;
                            do
                            {
                                uVar3 = (int)uVar5 + 1;
                                bVar6 = bVar6 - LookUpLut4[uVar5 + (long)hashHelp2 * 0xa8];
                                lVar9 = lVar9 + ((long)hashInBytes[uVar5 + indexIdkHelp] << bVar6);
                                uVar5 = (uint)uVar3;
                            }
                            while (uVar3 < hashHelp3);
                            uVar5 = 0;
                            ulong local_res8 = ((ulong)hashHelp2 << 0x38) + (ulong)lVar9;

                            if (hashLength + 1 == roundLength) // HACKFIX: This edge case will forever haunt me in my dreams
                            {
                                var grr = (ushort)local_res8;
                                local_res8 = local_res8 >> 16;
                                local_res8 += grr;
                            }

                            var outBytes = BitConverter.GetBytes(local_res8);
                            for (int i = 0; i < 8; i++)
                            {
                                hashingBytes[(indexGrr * 8) + i] = outBytes[i];
                            }

                            indexIdkHelp = indexIdkHelp + LookUpLut3[hashHelp2];

                            if (hashLength2 < 8)
                            {
                                hashLength2 = 0;
                            }
                            else
                            {
                                hashLength2 -= 8;
                            }
                        }
                    }
                    else
                    {
                        long lVar9 = (byte)uVar5 * 0xa8;

                    FirstLoop:

                        long hashHelp2 = 0;
                        do
                        {
                            hashHelp3 = LookUpLut3[uVar5];
                            var shiftBy = LookUpLut4[lVar9 + hashHelp2];
                            var shifted = 1L << shiftBy;
                            var comp = hashInBytes[hashHelp2 + indexIdkHelp];
                            if ((shifted <= comp))
                            {
                                break;
                            }
                            hashHelp2++;
                        }
                        while (hashHelp2 < hashHelp3);

                        if (hashHelp2 != hashHelp3)
                        {
                            do
                            {
                                uVar5 = uVar5 + 1;
                                lVar9 = (long)uVar5 * 0xa8;
                                var shiftBy = LookUpLut4[lVar9 + hashHelp2];
                                var shifted = 1L << shiftBy;
                                var comp = hashInBytes[hashHelp2 + indexIdkHelp];
                                if (comp < shifted)
                                {
                                    break;
                                }
                            }
                            while (hashHelp2 < LookUpLut3[uVar5]);
                            goto FirstLoop;
                        }
                        lVar9 = 0;
                        int bVar6 = 0x38;
                        hashHelp2 = 0;
                        uint uVar3 = 0;
                        do
                        {
                            uVar3 = (uint)hashHelp2 + 1;
                            var hhhhh = LookUpLut4[(long)uVar5 * 0xa8 + hashHelp2];
                            bVar6 = bVar6 - hhhhh;
                            long aaaa = hashInBytes[hashHelp2 + indexIdkHelp];
                            long bbbb = aaaa << bVar6;
                            lVar9 = lVar9 + bbbb;
                            hashHelp2 = uVar3;
                        } while (uVar3 < hashHelp3);
                        hashHelp2 = 0;
                        ulong local_res8 = ((ulong)uVar5 << 0x38) + (ulong)lVar9;

                        var outBytes = BitConverter.GetBytes(local_res8);
                        for (int i = 0; i < 8; i++)
                        {
                            hashingBytes[(indexGrr * 8) + i] = outBytes[i];
                        }
                        hashLength2 -= 8;
                        indexGrr++;
                        Debug.WriteLine(BitConverter.ToString(hashingBytes).Replace("-", " "));
                        indexIdkHelp = indexIdkHelp + LookUpLut3[uVar5];
                        if ((local_res8 == (ulong)hashHelp2))
                        {

                        }
                    }
                }
                while (indexIdkHelp < hashLength);
            }

            Debug.WriteLine(Convert.ToHexString(hashingBytes));

            List<byte> outHash = beginBytes.ToList();
            outHash.AddRange(hashingBytes);

            return outHash.ToArray();
        }

        #endregion


        private static long GetByteLength(byte[] hashBytes)
        {
            long iVar1;
            iVar1 = 0;
            if (hashBytes[0] != 0)
            {
                iVar1 = -1;
                do
                {
                    iVar1 = iVar1 + 1;
                } while (hashBytes[iVar1 * 2] != 0 || hashBytes[(iVar1 * 2) + 1] != 0);
            }
            return iVar1;
        }

        [DllImport("advapi32.dll", EntryPoint = "RegQueryInfoKey", CallingConvention = CallingConvention.Winapi, SetLastError = true)]
        extern private static int RegQueryInfoKey(
        SafeRegistryHandle handle,
        out StringBuilder lpClass,
        out uint lpcbClass,
        nint lpReserved,
        nint lpcSubKeys,
        nint lpcbMaxSubKeyLen,
        nint lpcbMaxClassLen,
        nint lpcValues,
        nint lpcbMaxValueNameLen,
        nint lpcbMaxValueLen,
        nint lpcbSecurityDescriptor,
        out FILETIME lpftLastWriteTime);

        [StructLayout(LayoutKind.Sequential)]
        public struct FILETIME
        {
            public uint DateTimeLow;
            public uint DateTimeHigh;
        }

        [DllImport("kernel32.dll",
        CallingConvention = CallingConvention.Winapi, SetLastError = true)]
        static extern bool FileTimeToSystemTime([In] ref FILETIME lpFileTime, out SYSTEMTIME lpSystemTime);

        [DllImport("kernel32.dll")]
        static extern bool SystemTimeToFileTime([In] ref SYSTEMTIME lpSystemTime, out FILETIME lpFileTime);

        // SYSTEMTIME
        [StructLayout(LayoutKind.Sequential)]
        private struct SYSTEMTIME
        {
            public ushort Year;
            public ushort Month;
            public ushort DayOfWeek;
            public ushort Day;
            public ushort Hour;
            public ushort Minute;
            public ushort Second;
            public ushort Milliseconds;
        }

        public static string? GetMachineID()
        {
            // Get the machine ID from the registry SQMClient
            var regKey = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\SQMClient");
            var machineId = regKey?.GetValue("MachineId")?.ToString();
            return machineId;
        }


        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        private delegate void CalculateHashStringFunc(IntPtr hashValPoint, string? machineId, string userSid, string fileType, string progId, SYSTEMTIME timestamp, out string hashValOut);
        static CalculateHashStringFunc CalculateHashString;

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        private delegate void HashThing1Func(string hashingString, int hashingStringLength, IntPtr hashedOut);
        static HashThing1Func HashThing1;

        static byte[] FuncProloguePattern        = { 0x48, 0x89, 0x5c, 0x24, 0x08, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x8d };

        static byte[] CalculateHashStringPattern = { 0x6c, 0x24, 0xf1 };
        static byte[] HashThing1Pattern     = { 0xac, 0x24, 0x90, 0xd8, 0xff, 0xff };


        static string[] HashValidationStrings = new string[3];
        static Dictionary<string, string> HashValidationStringHashes = new()
        {
            { "{3822b7ca-c2f4-4889-b8cc-4ce39a8fb81c}", "b00567862f7d98e19852d440a015240c" },
            { "{d185e0a1-e265-4724-aa21-3a17b038d72e}", "25109076282e50ca419f1fdd0066a1a1" },
            { "{97b6bcf4-c367-4577-95be-73bd3053a5e0}", "467c6722ee84ac9b86a33fd34e62378b" }
        };

        static byte[] HashValidationSearchPattern = { 0x43, 0x00, 0x6F, 0x00, 0x70, 0x00, 0x79, 0x00 };
        static int HashValidationStringSearchLength = 83;

        static int[][] StringOrders =
        [
            [4,0,3,5,2,1],
            [4,3,0,1,5,2],
            [1,3,4,0,5,2],
        ];


        public static void WriteExtensionKeys(string progId, string extension)
        {
            var machineId = GetMachineID();
            machineId = machineId.TrimStart('{').TrimEnd('}'); // Remove curly braces if present
            IntPtr hashPtr = Marshal.AllocHGlobal(16);
            IntPtr libraryHandle = NativeMethods.LoadLibrary("Windows.Internal.OpenWithHost.dll");
            var libBytes = File.ReadAllBytes(@"C:\Windows\System32\Windows.Internal.OpenWithHost.dll");

            try
            {
                var keyPath = @$"Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\{extension}\UserChoiceLatest";
                var key = Registry.CurrentUser.CreateSubKey(keyPath);
                var key2 = key.CreateSubKey("ProgId");
                key2.SetValue("ProgId", progId);

                var hTest = RegQueryInfoKey(key2.Handle, out StringBuilder lpClass, out _, 0, 0, 0, 0, 0, 0, 0, 0, out FILETIME lpftLastWriteTime);

                var testStamp = FileTimeToSystemTime(ref lpftLastWriteTime, out SYSTEMTIME lpSystemTime); // Gotta do a roundabout here because for some reason it converts it to UTC
                var aaaa = SystemTimeToFileTime(ref lpSystemTime, out FILETIME timestamp);                // Or the other way around, idk and idc

                var hash2 = GetHashNew(machineId, SFTA.userSID, extension, progId, timestamp);

                FileTimeToSystemTime(ref lpftLastWriteTime, out SYSTEMTIME systemTime2);

                key.SetValue("Hash", hash2);
                if(CalculateHashString != null)
                {
                    CalculateHashString(IntPtr.Zero, machineId, SFTA.userSID, extension, progId, systemTime2, out string hash3);

                    if(hash3 != hash2)
                    {
                        Debug.WriteLine("Hash mismatch!");
                        Debug.WriteLine($"Generated: {hash2}");
                        Debug.WriteLine($"Expected:  {hash3}");
                    }
                }
                Console.WriteLine($"Set {extension} to {progId} with hash {hash2}");
            }
            catch (Exception e)
            {
                Console.WriteLine("Error setting extension keys");
                Console.WriteLine(e);
            }
        }

        public static string GetHexDateTime(FILETIME timestamp)
        {
            var hi = timestamp.DateTimeHigh;
            var low = timestamp.DateTimeLow;
            return $"{hi:X8}{low:X8}".ToLower();
        }

        public static string GetHashNew(string? machineId, string userSid, string fileType, string programId, FILETIME timestamp)
        {
            uint machineHash; // So, this one is a value between 0 and 2 and it determines which hash validation string is used in hashing
            if (string.IsNullOrEmpty(machineId) || machineId.Length == 0)
            {
                machineHash = 0; // No machine ID, use a default value
            }
            else
            {
                machineHash = (uint)machineId[^1] % 3;
            }

            long[] hashingArr = new long[7];
            var dateTimeHex = GetHexDateTime(timestamp);

            string[] hashParamArr =
            {
                fileType,
                userSid,
                programId,
                dateTimeHex,
                HashValidationStrings[machineHash],
                machineId
            };

            var hashParamOrder = StringOrders[machineHash];

            var hashedString = "";

            for (int i = 0; i < 6; i++)
            {
                hashedString += hashParamArr[hashParamOrder[i]]; // Yes really, even the order changes depending on MachineID
            }

            IntPtr hashOut1 = Marshal.AllocHGlobal(24);
            Marshal.WriteInt64(hashOut1, 0);
            Marshal.WriteInt64(hashOut1, 8, -1);
            Marshal.WriteInt64(hashOut1, 16, -1);
            
            if(HashThing1 != null)
            {
                HashThing1(hashedString.ToLowerInvariant(), hashedString.Length, hashOut1); // This is the actual hashing funciton, for comparison
            }

            var hashout = NewHash.CalculateHash(hashedString.ToLowerInvariant());

            var hashOut2 = Marshal.ReadIntPtr(hashOut1, 0);
            var hahaha = Marshal.ReadInt64(IntPtr.Add(hashOut1, 0x8));
            byte[] hashBytes = new byte[hashedString.Length + 8];
            Marshal.Copy(hashOut2, hashBytes, 0, hashedString.Length + 8);
            IntPtr outHash = Marshal.AllocHGlobal(0x40);

            Debug.WriteLine(BitConverter.ToString(hashBytes).Replace("-", " "));
            Debug.WriteLine(BitConverter.ToString(hashout).Replace("-", " "));

            var outHashByteLen = hahaha;

            if (hahaha == -1)
            {
                outHashByteLen = GetByteLength(hashBytes);
            }

            var hashByteLen = (int)(outHashByteLen * 2) + 2;

            var outHashBytes1 = hashBytes[0..hashByteLen];
            var outHashBytes2 = hashout[0..hashByteLen];

            var aaa = SFTA.GetHash(outHashBytes1, hashByteLen); 
            var bbb = SFTA.GetHash(outHashBytes2, hashByteLen);

            if (aaa != bbb)
            {
                Debug.WriteLine("Uh oh");
            }

            return bbb;
        }

    }
}
