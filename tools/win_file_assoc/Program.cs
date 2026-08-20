// CLI helper: escribe UserChoice + UserChoiceLatest en Windows 10/11.
// Basado en PS-SFTA / DefaultApps (MIT). Solo se invoca desde WinFileAssociation.cpp.
using System;

namespace DefaultApps
{
    internal static class Program
    {
        private static int Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.Error.WriteLine("usage: LGA_WinSetFTA <extension> <progId>");
                return 2;
            }

            var extension = args[0];
            if (!extension.StartsWith("."))
            {
                extension = "." + extension;
            }

            try
            {
                SFTA.SetFTA(extension, args[1]);
                return 0;
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine(ex.Message);
                return 1;
            }
        }
    }
}
