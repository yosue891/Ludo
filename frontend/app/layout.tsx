import type { Metadata, Viewport } from "next";
import "./globals.css";
import "./game.css";

export const metadata: Metadata = {
  title: "Ludo Live",
  description: "Ludo multijugador en tiempo real"
};

export const viewport: Viewport = {
  width: "device-width",
  initialScale: 1,
  themeColor: "#0d0f12"
};

export default function RootLayout({ children }: Readonly<{ children: React.ReactNode }>) {
  return (
    <html lang="es">
      <body>{children}</body>
    </html>
  );
}
