'************************************************************************************
'*  Copyright (C) 2007  RenEvo Software & Designs
'************************************************************************************
'*  This program is free software; you can redistribute it and/or
'*  modify it under the terms of the GNU General Public License
'*  as published by the Free Software Foundation; either version 2
'*  of the License, or (at your option) any later version.
'*  
'*  This program is distributed in the hope that it will be useful,
'*  but WITHOUT ANY WARRANTY; without even the implied warranty of
'*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
'*  GNU General Public License for more details.
'*  
'*  You should have received a copy of the GNU General Public License
'*  along with this program; if not, write to the Free Software
'*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
'************************************************************************************

'Documentation: Logging.LogTraceListener

Public Class MainGUI

    Private Sub MainGUI_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Trace.WriteLine("Application Startup")
    End Sub

    Private Sub AboutToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles AboutToolStripMenuItem.Click
        'TODO: Replace this with a real dialog form
        Dim aboutString As New System.Text.StringBuilder()
        aboutString.AppendFormat("{0} version {1}, Copyright (C) {2} RenEvo Software & Designs. ", Application.ProductName, Application.ProductVersion, Now.Year)
        aboutString.AppendLine() : aboutString.AppendLine()
        aboutString.AppendFormat("{0} comes with ABSOLUTELY NO WARRANTY.", Application.ProductName)
        MessageBox.Show(aboutString.ToString, "About", MessageBoxButtons.OK, MessageBoxIcon.Information, MessageBoxDefaultButton.Button1)
    End Sub

    Private Sub ExitToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ExitToolStripMenuItem.Click
        Application.Exit()
    End Sub

    Private Sub LogFileToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles LogFileToolStripMenuItem.Click
        Process.Start(String.Format("{0}\notepad.exe", Environment.GetFolderPath(Environment.SpecialFolder.System)), My.Application.Log.DefaultFileLogWriter.FullLogFileName)
    End Sub

End Class
