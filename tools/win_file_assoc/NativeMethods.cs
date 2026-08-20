using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace DefaultApps
{
    public static class NativeMethods
    {
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern IntPtr LoadLibrary(string lpFileName);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern IntPtr FindResource(IntPtr hModule, string lpName, string lpType);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern IntPtr FindResource(IntPtr hModule, IntPtr lpName, string lpType);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern IntPtr LoadResource(IntPtr hModule, IntPtr hResInfo);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern IntPtr LockResource(IntPtr hResData);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern uint SizeofResource(IntPtr hModule, IntPtr hResInfo);


        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true, BestFitMapping = false, ThrowOnUnmappableChar = true)]
        internal static extern int LoadString(IntPtr hInstance, uint wID, StringBuilder lpBuffer, int nBufferMax);


        [DllImport("kernel32.dll")]
        public static extern int FreeLibrary(IntPtr hLibModule);

        /// <summary>Returns a string resource from a DLL.</summary>
        /// <param name="DLLHandle">The handle of the DLL (from LoadLibrary()).</param>
        /// <param name="ResID">The resource ID.</param>
        /// <returns>The name from the DLL.</returns>
        public static string GetStringResource(IntPtr handle, uint resourceId)
        {
            StringBuilder buffer = new StringBuilder(8192);     //Buffer for output from LoadString()

            int length = LoadString(handle, resourceId, buffer, buffer.Capacity);

            return buffer.ToString(0, length);      //Return the part of the buffer that was used.
        }


        public static string GetStringResource(string dllPath, string resourceName)
        {
            IntPtr hModule = NativeMethods.LoadLibrary(dllPath);
            if (hModule == IntPtr.Zero)
            {
                throw new Exception("Failed to load library.");
            }

            IntPtr hResInfo = NativeMethods.FindResource(hModule, resourceName, "STRING");
            if (hResInfo == IntPtr.Zero)
            {
                throw new Exception("Failed to find resource.");
            }

            IntPtr hResData = NativeMethods.LoadResource(hModule, hResInfo);
            if (hResData == IntPtr.Zero)
            {
                throw new Exception("Failed to load resource.");
            }

            IntPtr pResource = NativeMethods.LockResource(hResData);
            if (pResource == IntPtr.Zero)
            {
                throw new Exception("Failed to lock resource.");
            }

            uint size = NativeMethods.SizeofResource(hModule, hResInfo);
            byte[] buffer = new byte[size];
            Marshal.Copy(pResource, buffer, 0, (int)size);

            return System.Text.Encoding.Unicode.GetString(buffer);
        }
        public static string GetStringResource(string resourcePath)
        {
            // Parse the resource path
            var parts = resourcePath.Split(new[] { ',' }, 2);
            if (parts.Length != 2 || !int.TryParse(parts[1], out int resourceId))
            {
                throw new ArgumentException("Invalid resource path format.");
            }

            string dllPath = parts[0].TrimStart('@');
            dllPath = Environment.ExpandEnvironmentVariables(dllPath);
            IntPtr hModule = NativeMethods.LoadLibrary(dllPath);
            if (hModule == IntPtr.Zero)
            {
                throw new Exception("Failed to load library.");
            }

            var str = GetStringResource(hModule, (uint)Math.Abs(resourceId));

            return str;

            IntPtr hResInfo = NativeMethods.FindResource(hModule, (IntPtr)resourceId, "STRING");
            if (hResInfo == IntPtr.Zero)
            {
                throw new Exception("Failed to find resource.");
            }

            IntPtr hResData = NativeMethods.LoadResource(hModule, hResInfo);
            if (hResData == IntPtr.Zero)
            {
                throw new Exception("Failed to load resource.");
            }

            IntPtr pResource = NativeMethods.LockResource(hResData);
            if (pResource == IntPtr.Zero)
            {
                throw new Exception("Failed to lock resource.");
            }

            uint size = NativeMethods.SizeofResource(hModule, hResInfo);
            byte[] buffer = new byte[size];
            Marshal.Copy(pResource, buffer, 0, (int)size);

            return Encoding.Unicode.GetString(buffer);
        }

    }
}
