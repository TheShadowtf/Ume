class Piesa 
{
    struct TipPiesa
    {
        int PiesaCurbata1 = 0;
    };
    
    public:
        Piesa(int _x, int _y, TipPiesa _piesa)
        {
            x = _x;
            y = _y;
            piesa = _piesa;
        }

        void draw()
        {
            switch (piesa)
            {
            case /* constant-expression */:
                setlinestyle(SOLID_LINE, 0, 3); // Linie groasă
                arc(x, y + size, 0, 90, size / 2); // Curba de colț îngroșată

                setlinestyle(SOLID_LINE, 0, 1); // Linie normală
                arc(x + size, y, 180, 270, size / 2); // Linie de legătură subțire
                break;
            
            default:
                break;
            }
        }

        void connect()
        {
            for (int i = 0; i < 4; i++)
            {
                for (int i = 0; i < 4; i++)
                {
                    
                }
            }
        }
    private:
        int x, y;
        int size = 50;
        TipPiesa piesa;
};

void deseneazatabel(int dimensiune, int cellSize)
{
    int screenWidth = getmaxx(); //getmaxx si getmaxy sunt folosite pt a obține dimensiunile maxime ale ferestrei grafice curente
    int screenHeight = getmaxy(); //getmaxx returneaza coordonata maxima a lui x, iar getmaxy returneaza coordonata maxima a lui y


    int xStart = (screenWidth - dimensiune * cellSize) / 2;
    int yStart = (screenHeight - dimensiune * cellSize) / 2;


    for (int i = 0; i <= dimensiune; i++)
    {
        line(xStart, yStart + i * cellSize, xStart + dimensiune * cellSize, yStart + i * cellSize);
        line(xStart + i * cellSize, yStart, xStart + i * cellSize, yStart + dimensiune * cellSize);
    }


    for (int i = 1; i <= dimensiune; i++)
    {
        for (int j = 1; j <= dimensiune; j++)
        {
            int x1 = xStart + (j - 1) * cellSize;
            int y1 = yStart + (i - 1) * cellSize;
            int x2 = x1 + cellSize;
            int y2 = y1 + cellSize;
        }
    }
}

void nivelUsor()
{
    cleardevice();
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 2);

    deseneazatabel(dimensiuneTabelaMica, 50);

    // Coordonatele pieselor
    int pieceX = 50, pieceY = 450, pieceSize = 40;

    setlinestyle(SOLID_LINE, 0, 1);

    // Desenam piesa cu linii incrucisate
    rectangle(pieceX, pieceY, pieceX + pieceSize, pieceY + pieceSize);
    desenPiesaLiniiIncrucisate(pieceX, pieceY, pieceSize);
    outtextxy(pieceX + 10, pieceY + pieceSize + 10, "A");

    // Desenam piesa curbata
    rectangle(pieceX + 70, pieceY, pieceX + 70 + pieceSize, pieceY + pieceSize);
    desenPiesaCurba1(pieceX + 70, pieceY, pieceSize);
    outtextxy(pieceX + 80, pieceY + pieceSize + 10, "B");

    //Desenam piesa corbata nr.2
    rectangle(pieceX + 140, pieceY, pieceX + 140 + pieceSize, pieceY + pieceSize);
    desenPiesaCurba2(pieceX + 140, pieceY, pieceSize);
    outtextxy(pieceX + 155, pieceY + pieceSize + 10, "C");

    setlinestyle(SOLID_LINE, 0, 1);

    butonMeniuPrincipal();

    int piesaSelectata = 0;
    bool jocInceput = false;

    while (true)
    {
        if (ismouseclick(WM_LBUTTONDOWN))
        {
            int x = mousex();
            int y = mousey();
            clearmouseclick(WM_LBUTTONDOWN);
            stergeMesaj(300, 450, 480, 50);

            // Selectam piesa
            if (x > pieceX && x < pieceX + pieceSize && y > pieceY && y < pieceY + pieceSize)
            {
                piesaSelectata = 1; // Linii incrucisate
                if(limba==1) outtextxy(300, 450, "Ai selectat: Linii incrucisate");
                else outtextxy(300, 450, "You selected: Crossed lines");
            }
            else if (x > pieceX + 70 && x < pieceX + 70 + pieceSize && y > pieceY && y < pieceY + pieceSize)
            {
                piesaSelectata = 2; // Curba
                if(limba==1) outtextxy(300, 450, "Ai selectat: Curba 1");
                else outtextxy(300, 450, "You selected: Curve 1");
            }
            else if (x > pieceX + 140 && x < pieceX + 140 + pieceSize && y > pieceY && y < pieceY + pieceSize)
            {
                piesaSelectata = 3; // Curba
                if(limba==1) outtextxy(300, 450, "Ai selectat: Curba 2");
                else outtextxy(300, 450, "You selected: Curve 2");
            }

            // Plasam piesa pe tabla
            else if (piesaSelectata != 0)
            {
                int cellSize = 50;
                int xStart = (getmaxx() - dimensiuneTabelaMica * cellSize) / 2;
                int yStart = (getmaxy() - dimensiuneTabelaMica * cellSize) / 2;

                int xTabla = (x - xStart) / cellSize;
                int yTabla = (y - yStart) / cellSize;

                if (!jocInceput)
                {
                    if (piesaSelectata == 1 && esteColt1(xTabla, yTabla))
                    {
                        tabla1[xTabla][yTabla] = piesaSelectata;
                        plasarePiesa1(xTabla, yTabla, piesaSelectata);
                        marcheazaColtNegru1(xTabla, yTabla); // Coltul opus devine negru
                        jocInceput = true;
                        piesaSelectata = 0; // Resetam selectia dupa plasare
                    }
                    else
                    {
                        if(limba==1) outtextxy(300, 480, "Piesa cu linii incrucisate trebuie plasata in colt!");
                        else outtextxy(300, 480, "The cross-lined piece must be placed in the corner!");
                    }
                }
                else if (estePozitieValida1(xTabla, yTabla))
                {
                    tabla1[xTabla][yTabla] = piesaSelectata;
                    plasarePiesa1(xTabla, yTabla, piesaSelectata);
                    piesaSelectata = 0; // Resetam selectia dupa plasare
                }
                else
                {
                    if(limba==1) outtextxy(300, 480, "Pozitie invalida!");
                    else outtextxy(300, 480, "Invalid position!");
                }
            }

            // Verificam daca s-a apasat pe butonul pentru meniul principal
            else if (x > 10 && x < 150 && y > getmaxy() - 50 && y < getmaxy() - 10)
            {
                return;
            }
        }
    }
}