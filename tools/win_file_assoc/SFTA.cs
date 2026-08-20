using System;
using System.Collections.Generic;
using Microsoft.Win32;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Security.Principal;
using System.Text;

namespace DefaultApps
{
    // Copied and modified from PowerShell SFTA released under MIT License
    // https://github.com/DanysysTeam/PS-SFTA
    //
    // Authors  : Danyfirex & Dany3j
    // Version    : 1.2.0
    // Author(s)  : Danyfirex & Dany3j
    // Credits    : https://bbs.pediy.com/thread-213954.htm
    //              LMongrain - Hash Algorithm PureBasic Version
    // License    : MIT License
    // Copyright  : 2022 Danysys. <danysys.com>

    public static class SFTA
    {
        static SFTA()
        {
        }

        // We're gonna write a fake Application Association Toast Application IDs to the registry to fool Windows into thinking the user set the default app through it
        public static void SetFakeAppID(string extension, string appID)
        {
            var toastIDs = new List<string>();
            var allProgIDs = Registry.ClassesRoot.OpenSubKey(@$"{extension}\OpenWithProgids")?.GetValueNames() ?? [];
            foreach (var progID in allProgIDs)
            {
                toastIDs.Add(@$"{progID}_{extension}");
            }

            // Weird
            var allApplicationToasts = Registry.ClassesRoot.OpenSubKey(@$"{extension}\OpenWithList")?.GetSubKeyNames() ?? [];
            foreach (var app in allApplicationToasts)
            {
                toastIDs.Add(@$"Applications\{app}_{extension}");
            }


            var key = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\ApplicationAssociationToasts", true);

            try
            {
                foreach (var toastID in toastIDs)
                {
                    key.SetValue(toastID, 0);
                    Console.WriteLine($"Set {toastID} to 0");
                }
                Console.WriteLine($"Set toast {extension} to {appID}");
            }
            catch (Exception e)
            {
                Console.WriteLine("Error setting fake AppID");
                Console.WriteLine(e);
            }
        }

        [DllImport("Shell32.dll")]
        private static extern int SHChangeNotify(int eventId, int flags, IntPtr item1, IntPtr item2);

        private static int SHCNE_ASSOCCHANGED = 0x08000000;
        private static int SHCNF_IDLIST = 0x0000;

        public static void BroadcastRegistryChanges()
        {
            // Broadcast a file type association change event
            // SHCNF_IDLIST must be specified in flags. item1 and item2 are unused and must be null
            SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, IntPtr.Zero, IntPtr.Zero);
        }

        public static string GetUserSID()
        {
            WindowsIdentity identity = WindowsIdentity.GetCurrent();
            return identity.User.Value;
        }

        const string hardcodedUserExperience = "User Choice set via Windows User Experience {D18B6DD5-6124-4341-9318-804003BAFA0B}";
        const string userExperienceSearch = "User Choice set via Windows User Experience";

        public static string GetUserExperience()
        {
            var shell32 = File.ReadAllBytes(@"C:\Windows\System32\shell32.dll");
            var shell32String = Encoding.Unicode.GetString(shell32);
            var userExperienceIndex = shell32String.IndexOf(userExperienceSearch);
            if (userExperienceIndex == -1)
            {
                return hardcodedUserExperience;
            }
            var userExperience = shell32String.Substring(userExperienceIndex, hardcodedUserExperience.Length);
            return userExperience;
        }

        static string userExperience = GetUserExperience();
        public static string userSID = GetUserSID();

        static string GetHexDateTime()
        {
            var now = DateTime.Now;
            var dateTime = new DateTime(now.Year, now.Month, now.Day, now.Hour, now.Minute, 0);
            var fileTime = dateTime.ToFileTime();
            var hi = fileTime >> 32;
            var low = fileTime & 0xFFFFFFFF;
            return $"{hi:X8}{low:X8}".ToLower();
        }

        public static MD5 md5 = MD5.Create();


        public static string GetHash(byte[] bytesBaseInfo, int lengthBase)
        {
            // Helper methods
            int ShiftRight(long value, int count)
            {
                int iValue = (int)value;
                if ((iValue & 0x80000000) != 0)
                {
                    return (int)unchecked((iValue >> count) ^ 0xFFFF0000);
                }
                else
                {
                    return iValue >> count;
                }
            }

            int GetLong(byte[] bytes, int index = 0)
            {
                return BitConverter.ToInt32(bytes, index);
            }

            // Compute MD5 hash of bytesBaseInfo
            byte[] bytesMD5 = md5.ComputeHash(bytesBaseInfo);

            bool condition = ((lengthBase & 4) <= 1);
            int length = (condition ? 1 : 0) + (lengthBase >> 2) - 1;
            string hash = "";

            if (length > 1)
            {
                // Initialize variables
                int PDATA = 0;
                int CACHE = 0;
                int COUNTER = 0;
                int INDEX = 0;
                int H1 = 0;
                int H2 = 0;
                int OUTHASH1 = 0;
                int OUTHASH2 = 0;
                int M0 = 0;
                int M1 = 0;
                int M2 = 0;
                int M3 = 0;
                int M4 = 0;
                int M5 = 0;
                int M6 = 0;
                int M7 = 0;
                int M8 = 0;
                int M9 = 0;
                int M10 = 0;
                int M11 = 0;
                // First pass
                H1 = unchecked((GetLong(bytesMD5) | 1) + 0x69FB0000);
                H2 = unchecked((GetLong(bytesMD5, 4) | 1) + 0x13DB0000);
                INDEX = ShiftRight((length - 2), 1);
                COUNTER = INDEX + 1;

                while (COUNTER > 0) unchecked
                    {
                        M0 = GetLong(bytesBaseInfo, PDATA) + OUTHASH1;
                        M1 = GetLong(bytesBaseInfo, PDATA + 4);
                        PDATA += 8;
                        M3 = (H1 * M0) - (0x10FA9605 * ShiftRight(M0, 16));
                        M4 = (0x79F8A395 * M3) + (0x689B6B9F * ShiftRight(M3, 16));
                        M5 = ((int)0xEA970001 * M4) - (0x3C101569 * ShiftRight(M4, 16));
                        M6 = (M5) + (M1);
                        M10 = (H2 * M6) - (0x3CE8EC25 * ShiftRight(M6, 16));
                        M11 = (0x59C3AF2D * M10) - (0x2232E0F1 * ShiftRight(M10, 16));
                        OUTHASH1 = (0x1EC90001 * M11) + (0x35BD1EC9 * ShiftRight(M11, 16));
                        M8 = CACHE + M5;
                        OUTHASH2 = OUTHASH1 + M8;
                        CACHE = OUTHASH2;
                        COUNTER--;
                    }

                byte[] outHash = new byte[16];
                byte[] buffer = BitConverter.GetBytes((int)OUTHASH1);
                Array.Copy(buffer, 0, outHash, 0, buffer.Length);
                buffer = BitConverter.GetBytes((int)OUTHASH2);
                Array.Copy(buffer, 0, outHash, 4, buffer.Length);

                // Second pass
                CACHE = 0;
                OUTHASH1 = 0;
                PDATA = 0;
                H1 = unchecked(GetLong(bytesMD5) | 1);
                H2 = unchecked(GetLong(bytesMD5, 4) | 1);
                INDEX = ShiftRight((length - 2), 1);
                COUNTER = INDEX + 1;


                while (COUNTER > 0) unchecked
                    {
                        M0 = ((GetLong(bytesBaseInfo, PDATA) + OUTHASH1));
                        PDATA += 8;
                        M1 = M0 * H1;
                        M2 = ((int)0xB1110000 * M1) - (0x30674EEF * ShiftRight(M1, 16));
                        M3 = (0x5B9F0000 * M2) - (0x78F7A461 * ShiftRight(M2, 16));
                        M4 = (0x12CEB96D * ShiftRight(M3, 16)) - (0x46930000 * M3);
                        M5 = (0x1D830000 * M4) + (0x257E1D83 * ShiftRight(M4, 16));
                        M6 = H2 * (M5 + GetLong(bytesBaseInfo, PDATA - 4));
                        M7 = (0x16F50000 * M6) - (0x5D8BE90B * ShiftRight(M6, 16));
                        M8 = ((int)0x96FF0000 * M7) - (0x2C7C6901 * ShiftRight(M7, 16));
                        M9 = (0x2B890000 * M8) + (0x7C932B89 * ShiftRight(M8, 16));
                        OUTHASH1 = ((int)0x9F690000 * M9) - (0x405B6097 * ShiftRight(M9, 16));
                        OUTHASH2 = OUTHASH1 + CACHE + M5;
                        CACHE = OUTHASH2;
                        COUNTER--;
                    }

                buffer = BitConverter.GetBytes(OUTHASH1);
                Array.Copy(buffer, 0, outHash, 8, buffer.Length);
                buffer = BitConverter.GetBytes(OUTHASH2);
                Array.Copy(buffer, 0, outHash, 12, buffer.Length);

                // Final processing
                byte[] outHashBase = new byte[8];
                int hashValue1 = unchecked(GetLong(outHash, 8) ^ GetLong(outHash, 0));
                int hashValue2 = unchecked(GetLong(outHash, 12) ^ GetLong(outHash, 4));

                buffer = BitConverter.GetBytes(hashValue1);
                Array.Copy(buffer, 0, outHashBase, 0, buffer.Length);
                buffer = BitConverter.GetBytes(hashValue2);
                Array.Copy(buffer, 0, outHashBase, 4, buffer.Length);

                hash = Convert.ToBase64String(outHashBase);
            }

            return hash;
        }


        public static string GetHash(string baseInfo)
        {
            
            return GetHash(Encoding.Unicode.GetBytes(baseInfo), (baseInfo.Length * 2) + 2);
        }

        [DllImport("advapi32.dll", SetLastError = true)]
        private static extern int RegOpenKeyEx(UIntPtr hKey, string subKey, int ulOptions, int samDesired, out UIntPtr hkResult);

        [DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern uint RegDeleteKey(UIntPtr hKey, string subKey);

        public static void DeleteKey(string key)
        {
            UIntPtr hKey = UIntPtr.Zero;
            RegOpenKeyEx((UIntPtr)0x80000001u, key, 0, 0x20019, out hKey);
            RegDeleteKey((UIntPtr)0x80000001u, key);
        }
        public static void WriteExtensionKeys(string progId, string extension, string hash)
        {
            try
            {
                var keyPath = @$"Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\{extension}\UserChoice";
                Console.WriteLine($"Remove Extension {keyPath}");
                DeleteKey(keyPath);
            }
            catch (Exception e)
            {
                Console.WriteLine("Error removing extension keys");
                Console.WriteLine(e);
            }

            try
            {
                var keyPath = @$"Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\{extension}\UserChoice";
                var key = Registry.CurrentUser.CreateSubKey(keyPath);
                key.SetValue("Hash", hash);
                key.SetValue("ProgId", progId);
                Console.WriteLine($"Set {extension} to {progId}");
            }
            catch (Exception e)
            {
                Console.WriteLine("Error setting extension keys");
                Console.WriteLine(e);
            }
        }

        public static void SetFTA(string extension, string progId)
        {
            Console.WriteLine($"Getting hash for {progId}, {extension}");
            var userDateTime = GetHexDateTime();
            //userDateTime = "01db520e5e240400";
            var baseInfo = $"{extension}{userSID}{progId}{userDateTime}{userExperience}\0".ToLowerInvariant();

            Console.WriteLine($"Base Info: {baseInfo}");
            try
            {
                var hash = GetHash(baseInfo);
                //Console.WriteLine($"Hash: {hash}");
                SetFakeAppID(extension, progId);
                WriteExtensionKeys(progId, extension, hash);
            }
            catch
            {

            }
            NewHash.WriteExtensionKeys(progId, extension);
            BroadcastRegistryChanges();
        }
    }
}