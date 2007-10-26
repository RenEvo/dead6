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

#Region " Events "

    Private Sub MainGUI_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        'load the plugins
        For Each plugin As Interfaces.IPlugin In My.Application.Plugins.Plugins
            If plugin.IsUICapable AndAlso plugin.Control IsNot Nothing Then
                With Me.ViewToolStripMenuItem.DropDownItems.Add(plugin.Name)
                    .Tag = plugin
                    AddHandler .Click, AddressOf PluginToolStripMenuItem_Click
                End With
            End If
        Next

        'see if we want to show the View menu
        Me.ViewToolStripMenuItem.Enabled = (Me.ViewToolStripMenuItem.DropDownItems.Count > 0)
    End Sub

    Private Sub PluginToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)
        If sender IsNot Nothing AndAlso TryCast(sender, ToolStripMenuItem) IsNot Nothing Then
            ToggleActiveUIElement(DirectCast(DirectCast(sender, ToolStripMenuItem).Tag, Interfaces.IPlugin).Control)
        End If
    End Sub

    Private Sub AboutToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles AboutToolStripMenuItem.Click
        Using aDiag As New AboutDialog()
            aDiag.ShowDialog(Me)
        End Using
    End Sub

    Private Sub ExitToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ExitToolStripMenuItem.Click
        Application.Exit()
    End Sub

    Private Sub LogFileToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles LogFileToolStripMenuItem.Click
        Process.Start(String.Format("{0}\notepad.exe", Environment.GetFolderPath(Environment.SpecialFolder.System)), String.Format("{0}\RenEvo Software & Designs\The Dead Six Tools\{1}\{2}{3:00000}.log", Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "1.0.0.0", "RenEvo.Dead6.Tools", System.IO.Directory.GetFiles(String.Format("{0}\RenEvo Software & Designs\The Dead Six Tools\{1}\", Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "1.0.0.0"), "*.log").Length - 1))
    End Sub

#End Region

#Region " Methods "

    ''' <summary>
    ''' Method to display a new control in the main workspace
    ''' </summary>
    ''' <param name="newElement">The control to display</param>
    Private Sub ToggleActiveUIElement(ByVal newElement As Control)
        'check to make sure that the workspace does not already have the control displayed
        If newElement IsNot Nothing AndAlso Me.pnlWorkspace.Controls.Contains(newElement) = False Then
            'hide the new one, so it doesn't "pop" into view
            newElement.Visible = False
            'add it to the workspace
            Me.pnlWorkspace.Controls.Add(newElement)
            'dock fill it
            newElement.Dock = DockStyle.Fill
            'show the new one
            newElement.Show()
            'bring it to front
            newElement.BringToFront()
            'check to see if we had a previous control
            If Me.pnlWorkspace.Controls.Count = 2 Then
                'hide the control
                Me.pnlWorkspace.Controls(0).Hide()
                'remove it
                Me.pnlWorkspace.Controls.RemoveAt(0)
            End If
        End If
    End Sub

#End Region

End Class
